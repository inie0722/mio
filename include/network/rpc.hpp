#pragma once

#include <functional>
#include <unordered_map>
#include <memory>
#include <string>

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include <iostream>

namespace libcpp
{
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
            //uint64_t消息大小| bool 调用是否成功 | 返回值
            //----------------------------------------------
            constexpr size_t BUF_SIZE = 4096;

            typedef std::function<void(void *socket, void *arg, uint32_t arg_size, void *ret, uint32_t *ret_size)> Handler;

            class service
            {
            private:
                //远程调用函数
                std::unordered_map<std::string, Handler> handler;

            public:
                class session : public std::enable_shared_from_this<session>
                {
                private:
                    friend class service;
                    std::unordered_map<std::string, Handler> &handler;
                    std::shared_ptr<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>> socket;

                    session(std::shared_ptr<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>> &socket, std::unordered_map<std::string, Handler> &handler) : handler(handler), socket(socket)
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
                        char arg[BUF_SIZE];
                        char ret[BUF_SIZE];

                        uint32_t arg_size;
                        uint32_t ret_size;

                        try
                        {
                            while (1)
                            {
                                ret_size = BUF_SIZE;
                                size_t length = socket->async_read_some(buffer(arg, BUF_SIZE), yield);
                                uint32_t size;
                                memcpy(&size, arg, sizeof(size));

                                while (length < size)
                                {
                                    length += socket->async_read_some(buffer(&arg[length], size - length), yield);
                                }

                                std::string name(&arg[sizeof(size)]);
                                uint32_t head_size = sizeof(size) + name.length() - +1;
                                arg_size = size - sizeof(size) - name.length() - 1;

                                ret_size -= sizeof(size) + 1;
                                try
                                {
                                    handler.at(name)(socket.get(), &arg[head_size], arg_size, &ret[sizeof(size) + 1], &ret_size);
                                    ret[sizeof(size)] = true;
                                }
                                //无对应函数
                                catch (const std::exception &e)
                                {
                                    std::cerr << e.what() << '\n';
                                    ret[sizeof(size)] = false;
                                }

                                ret_size += sizeof(size) + 1;
                                memcpy(ret, &ret_size, sizeof(ret_size));
                                socket->async_write(buffer(ret, ret_size), yield);
                            }
                        }
                        catch (const std::exception &e)
                        {
                            std::cerr << e.what() << '\n';
                        }
                    }
                };

                session *make_session(std::shared_ptr<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>> socket)
                {
                    return new session(socket, handler);
                }

                //绑定远程调用函数
                void bind(const std::string &name, Handler handler)
                {
                    this->handler[name] = handler;
                }
            };

            class client : public std::enable_shared_from_this<client>
            {
            private:
                std::shared_ptr<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>> socket;

            public:
                client(std::shared_ptr<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>> &socket) : socket(socket)
                {
                }

                //远程调用
                bool call(const std::string &name, void *arg, uint32_t arg_size, void *ret, uint32_t *ret_size)
                {
                    using namespace boost::asio;

                    try
                    {
                        char buf[BUF_SIZE];
                        size_t head = sizeof(arg_size) + name.length() + 1;
                        arg_size += name.length() + 1 + sizeof(arg_size);

                        memcpy(buf, &arg_size, sizeof(arg_size));
                        memcpy(&buf[sizeof(arg_size)], name.c_str(), name.length() + 1);
                        memcpy(&buf[head], arg, arg_size);
                        socket->write(buffer(buf, arg_size));

                        char ret_head[sizeof(arg_size) + 1];
                        socket->read_some(buffer(&ret_head, sizeof(arg_size) + 1));

                        if (!ret_head[sizeof(arg_size)])
                        {
                            return false;
                        }

                        uint64_t size;
                        memcpy(&size, ret_head, sizeof(size));
                        socket->read_some(buffer(ret, size));
                    }
                    catch (const std::exception &e)
                    {
                        return false;
                    }

                    return true;
                }
            };
        } // namespace rpc
    }     // namespace network
} // namespace libcpp