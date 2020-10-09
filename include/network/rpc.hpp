#pragma once

#include "tool.hpp"

#include <functional>
#include <unordered_map>
#include <memory>
#include <string>

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include <iostream>

namespace network
{
    namespace rpc
    {
        //arg参数 数据包格式
        //-------------------------------
        //uint64_t消息大小| 函数名 | 参数
        //-------------------------------

        //ret参数 数据包格式
        //----------------------------------------------
        //bool 调用是否成功| uint64_t消息大小| 返回值
        //----------------------------------------------
        constexpr size_t BUF_SIZE = 4096;

        typedef std::function<bool(void *socket, void *arg, uint64_t arg_size, void *ret, uint64_t *ret_size)> Handler;

        class Service
        {
        private:
            //远程调用函数
            std::unordered_map<std::string, Handler> handler;

        public:
            class Session : public std::enable_shared_from_this<Session>
            {
            private:
                friend class Service;
                std::unordered_map<std::string, Handler> &handler;
                std::shared_ptr<boost::asio::ip::tcp::socket> socket;

                Session(std::shared_ptr<boost::asio::ip::tcp::socket> &socket, std::unordered_map<std::string, Handler> &handler) : handler(handler), socket(socket)
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
                    char arg[BUF_SIZE];
                    char ret[BUF_SIZE];

                    uint64_t arg_size;
                    uint64_t ret_size;

                    try
                    {
                        while (1)
                        {
                            ret_size = BUF_SIZE;
                            socket->async_receive(buffer(&arg_size, sizeof(arg_size)), yield);
                            arg_size = network::ntoh(arg_size);
                            socket->async_receive(buffer(arg, arg_size), yield);

                            std::string name(arg);
                            arg_size -=  name.length() + 1;

                            ret_size -= sizeof(ret_size) + 1;
                            try
                            {
                                if (!handler.at(name)(socket.get(), &arg[name.length() + 1], arg_size, &ret[sizeof(ret_size) + 1], &ret_size))
                                {
                                    //调用参数异常
                                    socket->close();
                                    break;
                                }

                                ret[0] = true;
                            }
                            //无对应函数
                            catch (const std::exception &e)
                            {
                                std::cerr << e.what() << '\n';
                                ret[0] = false;
                            }

                            ret_size += sizeof(ret_size) + 1;
                            auto t = ret_size;
                            ret_size = network::hton(ret_size);
                            memcpy(&ret[1], &ret_size, sizeof(ret_size));
                            ret_size = t;

                            //发送返回值
                            socket->async_send(buffer(ret, ret_size), yield);
                        }
                    }
                    catch (const std::exception &e)
                    {
                        std::cerr << e.what() << '\n';
                    }

                    return;
                }
            };

            Session *make_session(std::shared_ptr<boost::asio::ip::tcp::socket> socket)
            {
                return new Session(socket, handler);
            }

            //绑定远程调用函数
            void bind(const std::string &name, Handler handler)
            {
                this->handler[name] = handler;
            }
        };

        class Client : public std::enable_shared_from_this<Client>
        {
        private:
            std::shared_ptr<boost::asio::ip::tcp::socket> socket;

        public:
            Client(std::shared_ptr<boost::asio::ip::tcp::socket> &socket) : socket(socket)
            {
            }

            ~Client()
            {
                socket->close();
            }

            //远程调用
            bool call(const std::string &name, void *arg, uint64_t arg_size, void *ret, uint64_t *ret_size)
            {
                using namespace boost::asio;

                try
                {
                    char buf[BUF_SIZE];
                    uint64_t size =  arg_size + name.length() + 1;
                    auto t = size;
                    size = network::hton(size);
                    memcpy(buf, &size, sizeof(size));
                    size = t;

                    //发送参数
                    memcpy(&buf[sizeof(size)], name.c_str(), name.length() + 1);
                    memcpy(&buf[sizeof(size) + name.length() + 1], arg, arg_size);

                    socket->send(buffer(buf, size + sizeof(arg_size)));

                    //读取 标志位
                    bool flag;
                    socket->receive(buffer(&flag, sizeof(flag)));

                    if (!flag)
                    {
                        return false;
                    }

                    //读取返回值
                    socket->receive(buffer(ret_size, sizeof(ret_size)));
                    *ret_size = network::ntoh(*ret_size);
                    socket->receive(buffer(ret, *ret_size));
                }
                catch (const std::exception &e)
                {
                    std::cout << e.what() << std::endl;
                    return false;
                }

                return true;
            }
        };
    } // namespace rpc
} // namespace network
