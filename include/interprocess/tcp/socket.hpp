#pragma once

#include <string>

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>

#include "interprocess/tool/host_resolver.hpp"

namespace mio
{
    namespace interprocess
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

                void connect(const std::string &address)
                {
                    auto host = host_resolve(io_context_, address);

                    socket_.connect(host.begin()->endpoint());
                }

                void async_connect(const std::string &address, boost::asio::yield_context yield)
                {
                    auto host = async_host_resolve(io_context_, address, yield);

                    socket_.async_connect(host.begin()->endpoint(), yield);
                }

                size_t write_some(const char *data, size_t size)
                {
                    using namespace boost::asio;
                    return socket_.write_some(buffer(data, size));
                }

                size_t read_some(char *data, size_t size)
                {
                    using namespace boost::asio;
                    return socket_.read_some(buffer(data, size));
                }

                size_t async_write_some(const char *data, size_t size, boost::asio::yield_context yield)
                {
                    using namespace boost::asio;
                    return socket_.async_write_some(buffer(data, size), yield);
                }

                size_t async_read_some(char *data, size_t size, boost::asio::yield_context yield)
                {
                    using namespace boost::asio;
                    return socket_.async_read_some(buffer(data, size), yield);
                }

                size_t write(const char *data, size_t size)
                {
                    using namespace boost::asio;
                    return boost::asio::write(socket_, buffer(data, size));
                }

                size_t read(char *data, size_t size)
                {
                    using namespace boost::asio;
                    return boost::asio::read(socket_, buffer(data, size));
                }

                size_t async_write(const char *data, size_t size, boost::asio::yield_context yield)
                {
                    using namespace boost::asio;
                    return boost::asio::async_write(socket_, buffer(data, size), yield);
                }

                size_t async_read(char *data, size_t size, boost::asio::yield_context yield)
                {
                    using namespace boost::asio;
                    return boost::asio::async_read(socket_, buffer(data, size), yield);
                }

                void close()
                {
                    socket_.close();
                }

                ~socket() = default;
            };
        } // namespace tcp
    }     // namespace interprocess
} // namespace mio