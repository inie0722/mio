#pragma once

#include "rpc.hpp"
#include "event.hpp"

#include <boost/bind.hpp>
#include <thread>
#include <iostream>
#include <memory>

namespace network
{
    namespace mq
    {
        enum class CLIENT_TYPE
        {
            RPC = 0,
            EVENT,
        };

        class Service
        {
        private:
            std::vector<std::unique_ptr<boost::asio::io_context>> io_context;
            std::vector<std::thread> thread;
            size_t thread_size;

            //rpc 服务
            rpc::Service rpc;

            event::Service eve;

            std::function<bool(void *socket, void *arg, uint32_t arg_size)> accept_handler;
            std::function<void(void *socket)> close_handler;

            boost::asio::io_context &get_io_context()
            {
                static std::atomic<size_t> pos(0);
                return *io_context[pos.fetch_add(1) % thread_size];
            }

        public:
            Service(size_t thread_size) : thread_size(thread_size)
            {
                for (size_t i = 0; i < thread_size; i++)
                {
                    io_context.push_back(std::make_unique<boost::asio::io_context>(1));

                    thread.push_back(std::thread([this, i]() {
                        boost::asio::io_context::work work(*io_context[i]);
                        io_context[i]->run();
                    }));
                }
            }

            ~Service()
            {
                for (size_t i = 0; i < thread_size; i++)
                {
                    io_context[i]->stop();
                    thread[i].join();
                }
            }

            void on_accept(std::function<bool(void *socket, void *arg, uint32_t arg_size)> Handler)
            {
                accept_handler = Handler;
            }

            void on_close(std::function<void(void *socket)> handler)
            {
                close_handler = handler;
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
                            auto &io_context = get_io_context();
                            auto socket = std::make_shared<boost::asio::ip::tcp::socket>(io_context);

                            //接收一个客户端
                            acceptor.async_accept(*socket, yield);

                            //创建一个协程 执行新的客户端
                            boost::asio::spawn(io_context, [=](yield_context yield) {
                                //验证客户端
                                char buf[256];
                                uint64_t size;
                                socket->async_receive(buffer(&size, sizeof(uint64_t)), yield);
                                socket->async_receive(buffer(buf, size), yield);

                                //如果验证失败
                                if (!accept_handler(socket.get(), buf, size))
                                {
                                    socket->close();
                                    return;
                                }

                                //解析 连接类型 创建对应的会话程序
                                socket->async_receive(buffer(buf, 1), yield);
                                switch ((CLIENT_TYPE)buf[0])
                                {
                                case CLIENT_TYPE::RPC:
                                {
                                    //RPC
                                    std::unique_ptr<rpc::Service::Session> session(rpc.make_session(socket));
                                    session->run(yield);
                                    break;
                                }

                                case CLIENT_TYPE::EVENT:
                                {
                                    //订阅推送
                                    std::unique_ptr<event::Service::Session> session(eve.make_session(socket));
                                    session->run(yield);
                                    break;
                                }
                                }
                                close_handler(socket.get());
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
                eve.push(name, arg, size, close_handler);
            }
        };

        class Client
        {
        private:
            std::vector<std::unique_ptr<boost::asio::io_context>> io_context;
            std::vector<std::thread> thread;
            size_t thread_size;

            boost::asio::io_context &get_io_context()
            {
                static std::atomic<size_t> pos(0);
                return *io_context[pos.fetch_add(1) % thread_size];
            }

            std::function<void(void *socket, void *arg, uint64_t &arg_size)> connect_handler;
            std::function<void(void *socket)> close_handler;

        public:
            Client(size_t thread_size) : thread_size(thread_size)
            {
                for (size_t i = 0; i < thread_size; i++)
                {
                    io_context.push_back(std::make_unique<boost::asio::io_context>(1));

                    thread.push_back(std::thread([this, i]() {
                        boost::asio::io_context::work work(*io_context[i]);
                        io_context[i]->run();
                    }));
                }
            }

            ~Client()
            {
                for (size_t i = 0; i < thread_size; i++)
                {
                    io_context[i]->stop();
                    thread[i].join();
                }
            }

            void on_connect(std::function<void(void *socket, void *arg, uint64_t &arg_size)> handler)
            {
                connect_handler = handler;
            }

            void on_close(std::function<void(void *socket)> handler)
            {
                close_handler = handler;
            }

            std::shared_ptr<rpc::Client> make_rpc_client(const std::string &ip, uint16_t port)
            {
                using namespace boost::asio;
                using namespace boost::asio::ip;
                using namespace boost::beast;

                auto &io_context = get_io_context();
                auto socket = std::make_shared<tcp::socket>(io_context);

                socket->connect(tcp::endpoint(ip::address().from_string(ip), port));

                char buf[256];
                uint64_t buf_size = 256 - sizeof(buf_size);

                //进行登录验证
                connect_handler(socket.get(), &buf[sizeof(buf_size)], buf_size);
                memcpy(buf, &buf_size, sizeof(buf_size));
                socket->send(buffer(buf, buf_size + sizeof(buf_size)));

                char type = (char)CLIENT_TYPE::RPC;

                socket->send(buffer(&type, 1));
                return std::make_shared<rpc::Client>(socket);
            }

            std::shared_ptr<event::Client> make_event_client(const std::string &ip, uint16_t port)
            {
                using namespace boost::asio;
                using namespace boost::asio::ip;
                using namespace boost::beast;

                auto &io_context = get_io_context();
                auto socket = std::make_shared<tcp::socket>(io_context);

                socket->connect(tcp::endpoint(ip::address().from_string(ip), port));

                char buf[256];
                uint64_t buf_size = 256 - sizeof(buf_size);

                //进行登录验证
                connect_handler(socket.get(), &buf[sizeof(buf_size)], buf_size);
                memcpy(buf, &buf_size, sizeof(buf_size));
                socket->send(buffer(buf, buf_size + sizeof(buf_size)));

                //请求对应的模式
                buf[0] = (char)CLIENT_TYPE::EVENT;

                socket->send(buffer(buf, 1));

                auto ret = std::make_shared<event::Client>(socket);

                auto ptr = ret.get();
                boost::asio::spawn(io_context, [=](yield_context yield) {
                    ptr->run(yield);
                    close_handler(socket.get());
                });
                return ret;
            }
        };
    } // namespace mq
} // namespace network
