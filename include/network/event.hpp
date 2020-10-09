#pragma once

#include "tool.hpp"

#include <memory>
#include <list>
#include <unordered_map>
#include <string>
#include <shared_mutex>
#include <iostream>
#include <functional>

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

namespace network
{
    namespace event
    {
        constexpr size_t BUF_SIZE = 4096;
        enum class MSG_TYPE : int8_t
        {
            SUBSCRIPTION = 0,
            UNSUBSCRIBE,
        };

        class Service
        {
        private:
            typedef std::list<std::shared_ptr<boost::asio::ip::tcp::socket>> List;
            std::unordered_map<std::string, List> event_list;
            std::shared_mutex mutex;

        public:
            class Session : public std::enable_shared_from_this<Session>
            {
            private:
                friend class Service;
                std::unordered_map<std::string, List> &event_list;
                std::shared_ptr<boost::asio::ip::tcp::socket> socket;
                std::shared_mutex &mutex;

                Session(std::shared_ptr<boost::asio::ip::tcp::socket> &socket, std::unordered_map<std::string, List> &event_list, std::shared_mutex &mutex) : event_list(event_list), socket(socket), mutex(mutex)
                {
                }

            public:
                ~Session()
                {
                    std::cout << __func__ << std::endl;
                }

                void run(boost::asio::yield_context yield)
                {
                    using namespace boost::asio;
                    //已订阅事件 迭代器
                    std::unordered_map<std::string, List::iterator> sub_it;
                    try
                    {
                        while (1)
                        {
                            uint64_t size;
                            char buf[BUF_SIZE];
                            //读取size
                            socket->async_receive(buffer(&size, sizeof(size)), yield);
                            //转换成本机序
                            size = network::ntoh(size);

                            //读取数据
                            socket->async_receive(buffer(buf, size), yield);

                            //分析消息类型
                            MSG_TYPE msg_type = (MSG_TYPE)buf[0];
                            std::string name(&buf[1]);

                            mutex.lock();
                            try
                            {
                                switch (msg_type)
                                {
                                //订阅
                                case MSG_TYPE::SUBSCRIPTION:
                                {
                                    event_list.at(name).push_front(socket);
                                    sub_it[name] = event_list.at(name).begin();
                                    break;
                                }
                                //取消订阅
                                case MSG_TYPE::UNSUBSCRIBE:
                                {
                                    event_list.at(name).erase(sub_it.at(name));
                                    sub_it.erase(name);
                                    break;
                                }
                                }
                            }
                            catch (const std::exception &e)
                            {
                                std::cerr << e.what() << '\n';
                            }
                            mutex.unlock();
                        }
                    }
                    catch (const std::exception &e)
                    {
                        std::cerr << e.what() << '\n';
                        mutex.lock();
                        for (auto i = sub_it.begin(); i != sub_it.end(); i++)
                        {
                            event_list[i->first].erase(i->second);
                        }
                        mutex.unlock();
                    }
                }
            };

            Session *make_session(std::shared_ptr<boost::asio::ip::tcp::socket> socket)
            {
                return new Session(socket, event_list, mutex);
            }

            //注册事件
            void registered(const std::string &name)
            {
                event_list.insert({name, List()});
            }

            //推送事件
            void push(const std::string &name, void *arg, uint64_t size, std::function<void(void *socket)> handler)
            {
                using namespace boost::asio;

                char buf[BUF_SIZE];
                size += name.length() + 1;
                //转换成网络序
                auto t = size;
                size = network::hton(size);
                memcpy(buf, &size, sizeof(size));
                size = t;

                memcpy(&buf[sizeof(size)], name.c_str(), name.length() + 1);
                memcpy(&buf[sizeof(size) + name.length() + 1], arg, size - ( name.length() + 1));

                mutex.lock_shared();
                for (auto i = event_list[name].begin(); i != event_list[name].end(); ++i)
                {
                    /*
                    try
                    {
                        (**i).send(buffer(buf, size + sizeof(size)));
                    }
                    catch (...)
                    {
                        handler((*i).get());
                    }
                    */

                    boost::asio::spawn((**i).get_executor(), [=](yield_context yield) {
                        try
                        {
                            (**i).async_send(buffer(buf, size + sizeof(size)), yield);
                        }
                        catch (...)
                        {
                            handler((*i).get());
                        }
                    });
                }
                mutex.unlock_shared();
            }
        };

        class Client : public std::enable_shared_from_this<Client>
        {
        private:
            typedef std::function<void(void *socket, void *arg, uint64_t arg_size)> Handler;
            std::unordered_map<std::string, Handler> handler;
            std::shared_ptr<boost::asio::ip::tcp::socket> socket;

        public:
            Client(std::shared_ptr<boost::asio::ip::tcp::socket> &socket) : socket(socket)
            {
            }

            ~Client()
            {
                socket->close();
            }

            bool subscription(const std::string &name, Handler handler)
            {
                using namespace boost::asio;
                try
                {
                    this->handler[name] = handler;
                    char buf[BUF_SIZE];
                    uint64_t size = name.length() + 1 + 1;
                    //转换成网络序
                    auto t = size;
                    size = network::hton(size);
                    memcpy(buf, &size, sizeof(size));
                    size = t;

                    buf[sizeof(size)] = (char)MSG_TYPE::SUBSCRIPTION;

                    memcpy(&buf[sizeof(size) + 1], name.c_str(), name.length() + 1);
                    socket->send(buffer(buf, size + sizeof(size)));
                }
                catch (...)
                {
                    return false;
                }
                return true;
            }

            bool unsubscribe(const std::string &name)
            {
                using namespace boost::asio;
                try
                {
                    char buf[BUF_SIZE];
                    uint64_t size = name.length() + 1 + 1;
                    //转换成网络序
                    auto t = size;
                    size = network::hton(size);
                    memcpy(buf, &size, sizeof(size));
                    size = t;

                    buf[sizeof(size)] = (char)MSG_TYPE::UNSUBSCRIBE;
                    memcpy(&buf[sizeof(size) + 1], name.c_str(), name.length() + 1);

                    size = network::ntoh(size);
                    socket->send(buffer(buf, size + sizeof(size)));
                }
                catch (...)
                {
                    return false;
                }
                return true;
            }

            void run(boost::asio::yield_context yield)
            {
                using namespace boost::asio;
                char arg[BUF_SIZE];
                uint64_t arg_size;
                try
                {
                    while (1)
                    {
                        uint64_t size;

                        socket->async_receive(buffer(&size, sizeof(size)), yield);
                        //转换成本机
                        size = network::ntoh(size);
                        socket->async_receive(buffer(arg, size), yield);

                        std::string name(arg);
                        arg_size = size - name.length() - 1;

                        handler[name](socket.get(), &arg[name.length() + 1], arg_size);
                    }
                }
                //与服务器断开连接
                catch (...)
                {
                    return;
                }
            }
        };
    } // namespace event

} // namespace network
