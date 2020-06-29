#pragma once

#include "rpc.hpp"
#include "event.hpp"

#include <boost/bind.hpp>
#include <thread>
#include <iostream>

namespace libcpp
{
    namespace network
    {
            enum class CLIENT_TYPE
            {
                RPC = 0,
                EVENT,
            };

            class service
            {
            private:
                std::unique_ptr<boost::asio::io_context[]> io_context;
                std::unique_ptr<std::thread[]> thread;
                size_t thread_size;

                //rpc 服务
                rpc::service rpc;

                event::service eve;

                boost::asio::io_context &get_io_context()
                {
                    static std::atomic<size_t> pos(0);
                    return io_context[pos.fetch_add(1) % thread_size];
                }

            public:
                service(size_t thread_size) : io_context(new boost::asio::io_context[thread_size]), thread(new std::thread[thread_size]), thread_size(thread_size)
                {
                    for (size_t i = 0; i < thread_size; i++)
                    {
                        thread[i] = std::thread([this, i]() {
                            boost::asio::io_context::work work(io_context[i]);
                            io_context[i].run();
                        });
                    }
                }

                void run(const std::string &ip, uint16_t port)
                {
                    using namespace boost::asio;
                    using namespace boost::asio::ip;
                    using namespace boost::beast;

                    auto &acc_io = get_io_context();

                    boost::asio::spawn(acc_io, [&, ip, port](yield_context yield) {
                        try
                        {
                            tcp::acceptor acceptor(acc_io, tcp::endpoint(ip::address().from_string(ip), port));

                            //等待客户端连接
                            while (1)
                            {
                                boost::asio::ip::tcp::socket socket(get_io_context());

                                //接收一个客户端
                                acceptor.async_accept(socket, yield);

                                //转换成ws协议
                                std::shared_ptr<websocket::stream<tcp::socket>> ws = std::make_shared<websocket::stream<tcp::socket>>(std::move(socket));

                                //创建一个协程 执行新的客户端
                                boost::asio::spawn(socket.get_io_context(), [=](yield_context yield) {
                                    ws->async_accept(yield);

                                    //解析 连接类型
                                    char type;
                                    ws->async_read_some(buffer(&type, 1), yield);

                                    //创建对应的会话程序
                                    switch ((CLIENT_TYPE)type)
                                    {
                                    case CLIENT_TYPE::RPC:
                                    {
                                        //RPC
                                        std::unique_ptr<rpc::service::session> session(rpc.make_session(ws));
                                        session->run(yield);
                                        break;
                                    }

                                    case CLIENT_TYPE::EVENT:
                                    {
                                        //订阅推送
                                        std::unique_ptr<event::service::session> session(eve.make_session(ws));
                                        session->run(yield);
                                        break;
                                    }

                                    default:
                                        break;
                                    }
                                });
                            }
                        }
                        catch (const std::exception &e)
                        {
                            std::cerr << e.what() << '\n';
                        }
                    });
                }

                void bind(const std::string &name, rpc::Handler handler)
                {
                    rpc.bind(name, handler);
                }

                //注册事件
                void registered(const std::string &name)
                {
                    eve.registered(name);
                }

                void push(const std::string &name, void *arg, uint64_t size)
                {
                    eve.push(name, arg, size);
                }
            };

            class client
            {
            private:
                std::unique_ptr<boost::asio::io_context[]> io_context;
                std::unique_ptr<std::thread[]> thread;
                size_t thread_size;

                boost::asio::io_context &get_io_context()
                {
                    static std::atomic<size_t> pos(0);
                    return io_context[pos.fetch_add(1) % thread_size];
                }

            public:
                client(size_t thread_size) : io_context(new boost::asio::io_context[thread_size]), thread(new std::thread[thread_size]), thread_size(thread_size)
                {
                    for (size_t i = 0; i < thread_size; i++)
                    {
                        thread[i] = std::thread([this, i]() {
                            boost::asio::io_context::work work(io_context[i]);
                            io_context[i].run();
                        });
                    }
                }

                rpc::client *make_rpc_client(const std::string &ip, uint16_t port)
                {
                    using namespace boost::asio;
                    using namespace boost::asio::ip;
                    using namespace boost::beast;

                    tcp::socket socket(get_io_context());

                    //连接服务
                    socket.connect(tcp::endpoint(ip::address().from_string(ip), port));

                    //转换成ws协议
                    auto ws = std::make_shared<websocket::stream<tcp::socket>>(std::move(socket));
                    //完成ws握手
                    ws->handshake(ip, "/");

                    char type = (char)CLIENT_TYPE::RPC;

                    ws->write(buffer(&type, 1));
                    return new rpc::client(ws);
                }

                event::client *make_event_client(const std::string &ip, uint16_t port)
                {
                    using namespace boost::asio;
                    using namespace boost::asio::ip;
                    using namespace boost::beast;

                    tcp::socket socket(get_io_context());

                    //连接服务
                    socket.connect(tcp::endpoint(ip::address().from_string(ip), port));

                    //转换成ws协议
                    auto ws = std::make_shared<websocket::stream<tcp::socket>>(std::move(socket));
                    //完成ws握手
                    ws->handshake(ip, "/");

                    char type = (char)CLIENT_TYPE::EVENT;

                    ws->write(buffer(&type, 1));

                    auto ret = new event::client(ws);
                    boost::asio::spawn(socket.get_io_context(), boost::bind(&event::client::run, ret, _1));
                    return ret;
                }
            };
    }     // namespace network
} // namespace libcpp