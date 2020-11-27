#pragma once

#include "mq/basic.hpp"
#include "mq/msg.hpp"

#include <vector>
#include <memory>
#include <thread>
#include <array>
#include <tuple>

#include <boost/asio.hpp>

namespace mio
{
    namespace mq
    {
        class manager
        {
        private:
            std::vector<std::unique_ptr<boost::asio::io_context>> io_context_;
            std::vector<std::thread> thread_;
            size_t thread_size_;

            std::atomic<size_t> io_context_count_;

        protected:
            std::function<void(const std::exception &e)> exception_handler_;

            boost::asio::io_context &get_io_context()
            {
                return *io_context_[io_context_count_.fetch_add(1) % thread_size_];
            }

        public:
            manager(size_t thread_size) : thread_size_(thread_size)
            {
                for (size_t i = 0; i < thread_size; i++)
                {
                    io_context_.push_back(std::make_unique<boost::asio::io_context>(1));

                    thread_.push_back(std::thread([this, i]() {
                        boost::asio::io_context::work work(*io_context_[i]);

                        while (1)
                        {
                            try
                            {
                                io_context_[i]->run();
                                break;
                            }
                            catch (const std::exception &e)
                            {
                                exception_handler_(e);
                            }
                            catch (...)
                            {
                                exception_handler_(std::runtime_error("An exception of unknown type"));
                            }
                        }
                    }));
                }
            }

            void on_exception(decltype(exception_handler_) &handler)
            {
                exception_handler_ = handler;
            }

            virtual ~manager()
            {
                for (size_t i = 0; i < thread_size_; i++)
                {
                    io_context_[i]->stop();
                    thread_[i].join();
                }
            }
        };

        template <typename Protocol, template <typename> typename... Args>
        class service : public manager
        {
        public:
            typedef basic_service<Protocol> basic_service_t;
            typedef typename Protocol::socket_t socket_t;
            typedef typename Protocol::acceptor_t acceptor_t;

        private:
            template <typename T>
            struct type_list
            {
                typedef T type;
            };

            std::tuple<type_list<Args<Protocol>>...> tuple_;

            std::array<std::unique_ptr<basic_service_t>, sizeof...(Args)> service_;

            std::function<bool(void *socket, const msg_t &msg)> accept_handler_;
            std::function<void(void *socket)> close_handler_;

            template <size_t... I>
            void init_args(std::index_sequence<I...>)
            {
                ((service_[I] = std::make_unique<Args<Protocol>>()), ...);
            }

        public:
            service(size_t thread_size) : manager(thread_size)
            {
                init_args(std::make_index_sequence<sizeof...(Args)>{});
            }

            void on_accept(const decltype(accept_handler_) &handler)
            {
                accept_handler_ = handler;
            }

            void on_close(decltype(close_handler_) &handler)
            {
                close_handler_ = handler;
            }

            template <size_t I>
            auto &get_service()
            {
                return static_cast<typename std::remove_reference<decltype(std::get<I>(tuple_))>::type::type &>(*service_[I]);
            }

            void run(const std::string &address)
            {
                using namespace boost::asio;
                using namespace boost::asio::ip;

                auto &acc_io = get_io_context();
                boost::asio::spawn(acc_io, [&, address](yield_context yield) {
                    try
                    {
                        acceptor_t acceptor(acc_io);
                        acceptor.bind(address);

                        while (1)
                        {
                            auto &io_context = get_io_context();
                            auto socket = std::make_shared<socket_t>(io_context);

                            acceptor.async_accept(*socket, yield);

                            boost::asio::spawn(io_context, [=](yield_context yield) {
                                try
                                {
                                    uint64_t index;
                                    {
                                        msg_t msg;
                                        uint64_t msg_size;

                                        //读取验证参数
                                        socket->async_read(&msg_size, sizeof(msg_size), yield);
                                        msg.resize(msg_size);
                                        socket->async_read(&msg[0], msg_size, yield);

                                        bool flag = accept_handler_(socket.get(), msg);
                                        socket->async_write(&flag, sizeof(flag), yield);

                                        if (!flag)
                                        {
                                            socket->close();
                                            return;
                                        }

                                        socket->async_read(&index, sizeof(index), yield);
                                    }

                                    auto session = service_[index]->make_session(socket);
                                    session->run(yield);
                                }
                                catch (const std::exception &e)
                                {
                                    exception_handler_(e);
                                }
                                close_handler_(socket.get());
                            });
                        }
                    }
                    catch (const std::exception &e)
                    {
                        exception_handler_(e);
                    }
                });
            }
        };

        template <typename Protocol, template <typename> typename... Args>
        class client : public manager
        {
        public:
            typedef typename Protocol::socket_t socket_t;

        private:
            template <typename T>
            struct type_list
            {
                typedef T type;
            };

            std::tuple<type_list<Args<Protocol>>...> tuple_;

            std::function<void(void *socket, msg_t &arg)> connect_handler_;

        public:
            client(size_t thread_size) : manager(thread_size)
            {
            }

            void on_connect(const decltype(connect_handler_) &connect_handler)
            {
                connect_handler_ = connect_handler;
            }

            template <size_t I>
            auto make_client(const std::string &address)
            {
                using namespace boost::asio;
                using namespace boost::asio::ip;

                auto &io_context = get_io_context();
                auto socket = std::make_shared<socket_t>(io_context);

                socket->connect(address);

                msg_t arg;
                connect_handler_(socket.get(), arg);

                uint64_t arg_size = arg.size();
                socket->write(&arg_size, sizeof(arg_size));
                socket->write(&arg[0], arg_size);

                size_t index = I;
                socket->write(&index, sizeof(index));

                //登录成功标志位
                bool flag;
                socket->read(&flag, sizeof(flag));

                if (!flag)
                {
                    throw std::runtime_error("Server verification failed");
                }

                auto ret = std::make_shared<typename std::remove_reference<decltype(std::get<I>(tuple_))>::type::type>(socket);
                return ret;
            }
        };
    } // namespace mq
} // namespace mio