#pragma once

#include <string>

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>

#include "network/tcp/socket.hpp"
#include "network/tool/host_resolver.hpp"

namespace mio
{
    namespace network
    {
        namespace tcp
        {
            class acceptor
            {
            private:
                std::string address_;
                boost::asio::io_context &io_context_;

                boost::asio::ip::tcp::acceptor acceptor_;

            public:
                acceptor(boost::asio::io_context &io_context) : io_context_(io_context), acceptor_(io_context_)
                {
                }

                void bind(const std::string &address)
                {
                    using namespace boost::asio;
                    using namespace boost::asio::ip;

                    auto host = host_resolve(io_context_, address);

                    acceptor_.open(host.begin()->endpoint().protocol());
                    acceptor_.bind(host.begin()->endpoint());
                    acceptor_.listen();
                }

                void accept(socket &peer)
                {
                    acceptor_.accept(peer.socket_);
                }

                void async_accept(socket &peer, boost::asio::yield_context yield)
                {
                    acceptor_.async_accept(peer.socket_, yield);
                }

                void close()
                {
                    acceptor_.close();
                }
                ~acceptor() = default;
            };
        } // namespace tcp
    }     // namespace interprocess
} // namespace mio