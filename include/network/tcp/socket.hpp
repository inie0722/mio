#pragma once

#include <string>

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>

#include "network/tool/host_resolver.hpp"

namespace mio
{
    namespace network
    {
        namespace tcp
        {
            class socket
            {
            private:
                friend class acceptor;
                std::string address_;
                boost::asio::io_context &io_context_;

                boost::asio::ip::tcp::socket socket_;

            public:
                socket(boost::asio::io_context &io_context) : io_context_(io_context), socket_(io_context_)
                {
                }

                socket(socket && other) : io_context_(other.io_context_), socket_(std::move(other.socket_))
                {
                }

                void connect(const std::string &address)
                {
                    auto host = host_resolve(io_context_, address);

                    socket_.connect(host.begin()->endpoint());
                }

                template<typename Yield>
                void async_connect(const std::string &address, Yield& yield)
                {
                    auto host = host_resolve(io_context_, address);

                    socket_.async_connect(host.begin()->endpoint(), yield);
                }

                size_t write_some(const void *data, size_t size)
                {
                    using namespace boost::asio;
                    return socket_.write_some(buffer(data, size));
                }

                size_t read_some(void *data, size_t size)
                {
                    using namespace boost::asio;
                    return socket_.read_some(buffer(data, size));
                }

                template<typename Yield>
                size_t async_write_some(const void *data, size_t size, Yield& yield)
                {
                    using namespace boost::asio;
                    return socket_.async_write_some(buffer(data, size), yield);
                }

                template<typename Yield>
                size_t async_read_some(void *data, size_t size, Yield& yield)
                {
                    using namespace boost::asio;
                    return socket_.async_read_some(buffer(data, size), yield);
                }

                size_t write(const void *data, size_t size)
                {
                    using namespace boost::asio;
                    return boost::asio::write(socket_, buffer(data, size));
                }

                size_t read(void *data, size_t size)
                {
                    using namespace boost::asio;
                    return boost::asio::read(socket_, buffer(data, size));
                }

                template<typename Yield>
                size_t async_write(const void *data, size_t size, Yield& yield)
                {
                    using namespace boost::asio;
                    return boost::asio::async_write(socket_, buffer(data, size), yield);
                }

                template<typename Yield>
                size_t async_read(void *data, size_t size, Yield& yield)
                {
                    using namespace boost::asio;
                    return boost::asio::async_read(socket_, buffer(data, size), yield);
                }

                void close()
                {
                    socket_.close();
                }

                ~socket() = default;

                bool is_open() const
                {
                    return socket_.is_open();
                }
            };
        } // namespace tcp
    }     // namespace interprocess
} // namespace mio