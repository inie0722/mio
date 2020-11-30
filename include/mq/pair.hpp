#pragma once

#include <string>
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>

namespace mio
{
    namespace mq
    {
        namespace pair
        {
            template <typename Protocol>
            class server
            {
            public:
                typedef typename Protocol::socket_t socket_t;
                typedef typename Protocol::acceptor_t acceptor_t;

            private:
                boost::asio::io_context &io_context_;
                acceptor_t acceptor_;
                socket_t socket_;

                void accept()
                {
                    if (!socket_.is_open())
                    {
                        acceptor_.accept(socket_);
                    }
                }

                void accept(boost::asio::yield_context yield)
                {
                    if (!socket_.is_open())
                    {
                        acceptor_.accept(socket_, yield);
                    }
                }

            public:
                server(boost::asio::io_context &io_context) : io_context_(io_context), acceptor_(io_context), socket_(io_context)
                {
                }

                void bind(const std::string &address)
                {
                    acceptor_.bind(address);
                }

                void write(const void *data, size_t size)
                {
                    accept();

                    try
                    {
                        socket_.write(data, size);
                    }
                    catch (...)
                    {
                        socket_.close();
                        write(data, size);
                    }
                }

                void write(const void *data, size_t size, boost::asio::yield_context yield)
                {
                    accept(yield);

                    try
                    {
                        socket_.write(data, size, yield);
                    }
                    catch (...)
                    {
                        socket_.close();
                        write(data, size, yield);
                    }
                }

                void read(void *data, size_t size)
                {
                    accept();

                    try
                    {
                        socket_.read(data, size);
                    }
                    catch (...)
                    {
                        socket_.close();
                        read(data, size);
                    }
                }

                void read(void *data, size_t size, boost::asio::yield_context yield)
                {
                    accept(yield);

                    try
                    {
                        socket_.read(data, size, yield);
                    }
                    catch (...)
                    {
                        socket_.close();
                        read(data, size, yield);
                    }
                }

                void close()
                {
                    socket_.close();
                    acceptor_.close();
                }

                ~server()
                {
                    close();
                }
            };

            template <typename Protocol>
            class client
            {
            public:
                typedef typename Protocol::socket_t socket_t;
                typedef typename Protocol::acceptor_t acceptor_t;

            private:
                boost::asio::io_context &io_context_;
                socket_t socket_;

            public:
                client(boost::asio::io_context &io_context) : io_context_(io_context), socket_(io_context)
                {
                }

                void connect(const std::string &address)
                {
                    socket_.connect(address);
                }

                void connect(const std::string &address, boost::asio::yield_context yield)
                {
                    socket_.connect(address, yield);
                }

                void write(const void *data, size_t size)
                {
                    socket_.write(data, size);
                }

                void write(const void *data, size_t size, boost::asio::yield_context yield)
                {
                    socket_.write(data, size, yield);
                }

                void read(void *data, size_t size)
                {
                    socket_.read(data, size);
                }

                void read(void *data, size_t size, boost::asio::yield_context yield)
                {
                    socket_.read(data, size, yield);
                }

                void close()
                {
                    socket_.close();
                }

                ~client()
                {
                    close();
                }
            };

        } // namespace pair
    }     // namespace mq
} // namespace mio