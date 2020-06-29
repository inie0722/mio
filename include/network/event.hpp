#pragma once

#include "network.hpp"
#include "tool.hpp"

#include <memory>
#include <list>
#include <unordered_map>
#include <string>
#include <shared_mutex>
#include <iostream>

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

namespace libcpp
{
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

            class service
            {
            private:
                typedef std::list<boost::beast::websocket::stream<boost::asio::ip::tcp::socket> *> List;
                std::unordered_map<std::string, List> event_list;
                std::shared_mutex mutex;

            public:
                class session : public std::enable_shared_from_this<session>
                {
                private:
                    friend class service;
                    std::unordered_map<std::string, List> &event_list;
                    std::shared_ptr<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>> socket;
                    std::shared_mutex &mutex;

                    session(std::shared_ptr<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>> &socket, std::unordered_map<std::string, List> &event_list, std::shared_mutex &mutex) : event_list(event_list), socket(socket), mutex(mutex)
                    {
                    }

                public:
                    ~session()
                    {
                        std::cout << __func__ << std::endl;
                    }

                    void run(boost::asio::yield_context yield)
                    {
                        using namespace boost::asio;
                        //以订阅事件 迭代器
                        std::unordered_map<std::string, List::iterator> sub_it;
                        try
                        {
                            while (1)
                            {
                                char buf[BUF_SIZE];
                                size_t length = socket->async_read_some(buffer(buf, BUF_SIZE), yield);
                                uint32_t size;
                                memcpy(&size, buf, sizeof(size));
                                //转换成本机序
                                size = network::ntoh(size);

                                while (length < size)
                                {
                                    length += socket->async_read_some(buffer(&buf[length], size - length), yield);
                                }

                                MSG_TYPE msg_type = (MSG_TYPE)buf[sizeof(size)];
                                std::string name(&buf[sizeof(size) + 1]);

                                mutex.lock();
                                try
                                {
                                    switch (msg_type)
                                    {
                                    //订阅
                                    case MSG_TYPE::SUBSCRIPTION:
                                    {
                                        event_list.at(name).push_front(socket.get());
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

                session *make_session(std::shared_ptr<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>> socket)
                {
                    return new session(socket, event_list, mutex);
                }

                //注册事件
                void registered(const std::string &name)
                {
                    event_list.insert({name, List()});
                }

                //推送事件
                void push(const std::string &name, void *arg, uint32_t size)
                {
                    using namespace boost::asio;

                    char buf[BUF_SIZE];
                    size += name.length() + 1 + sizeof(size);
                    //转换成网络序
                    auto t = size;
                    size = network::hton(size);
                    memcpy(buf, &size, sizeof(size));
                    size = t;

                    memcpy(&buf[sizeof(size)], name.c_str(), name.length() + 1);
                    memcpy(&buf[name.length() + 1 + sizeof(size)], arg, size);

                    mutex.lock_shared();
                    for (auto i = event_list[name].begin(); i != event_list[name].end(); ++i)
                    {
                        try
                        {
                            (**i).write(buffer(buf, size));
                        }
                        catch (const std::exception &e)
                        {
                            std::cerr << e.what() << '\n';
                        }
                    }
                    mutex.unlock_shared();
                }
            };
            
            class client
            {
            private:
                typedef std::function<void(void *socket, void *arg, uint32_t arg_size)> Handler;
                std::unordered_map<std::string, Handler> handler;
                std::shared_ptr<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>> socket;

            public:
                client(std::shared_ptr<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>> &socket) : socket(socket)
                {
                }

                void subscription(const std::string &name, Handler handler)
                {
                    using namespace boost::asio;
                    this->handler[name] = handler;
                    char buf[BUF_SIZE];
                    uint32_t size = sizeof(size) + name.length() + 1 + 1;
                    //转换成网络序
                    auto t = size;
                    size = network::hton(size);
                    memcpy(buf, &size, sizeof(size));
                    size = t;

                    buf[sizeof(size)] = (char)MSG_TYPE::SUBSCRIPTION;
                    memcpy(&buf[sizeof(size) + 1], name.c_str(), name.length() + 1);
                    socket->write(buffer(buf, size));
                }

                void unsubscribe(const std::string &name)
                {
                    using namespace boost::asio;
                    char buf[BUF_SIZE];
                    uint64_t size = sizeof(size) + name.length() + 1;
                    //转换成网络序
                    auto t = size;
                    size = network::hton(size);
                    memcpy(buf, &size, sizeof(size));
                    size = t;

                    buf[sizeof(size)] = (char)MSG_TYPE::UNSUBSCRIBE;
                    memcpy(&buf[sizeof(size) + 1], name.c_str(), name.length() + 1);

                    size = network::ntoh(size);
                    socket->write(buffer(buf, size));
                }

                void run(boost::asio::yield_context yield)
                {
                    using namespace boost::asio;
                    char arg[BUF_SIZE];
                    uint32_t arg_size;

                    while (1)
                    {
                        size_t length = socket->async_read_some(buffer(arg, BUF_SIZE), yield);
                        uint32_t size;
                        memcpy(&size, arg, sizeof(size));
                        //转换成网络序
                        size = network::ntoh(size);

                        while (length < size)
                        {
                            length += socket->async_read_some(buffer(&arg[length], size - length), yield);
                        }

                        std::string name(&arg[sizeof(size)]);
                        arg_size = size - sizeof(size) - name.length() - 1;

                        handler[name](socket.get(), &arg[sizeof(size)], arg_size - sizeof(size));
                    }
                }
            };
        } // namespace event

    } // namespace network
} // namespace libcpp