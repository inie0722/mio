#pragma once

#include <string>
#include <memory>

#include <boost/asio.hpp>

#include "mq/detail/round_robin.hpp"

namespace mio
{
    namespace mq
    {
        namespace protocol
        {
            template <typename Protocol>
            class transfer
            {
            private:
                using socket_t = Protocol::socket_t;
                using acceptor_t = Protocol::acceptor_t;

            public:
                class socket : public basic_socket, private socket_t
                {
                public:
                    socket(boost::asio::io_context &io_context) : Socket(io_context)
                    {
                    }

                    virtual ~socket() = default;

                    virtual void connect(const std::string &address)
                    {
                        Socket::async_connect(address, boost::fibers::asio::yield);
                    }

                    virtual size_t write(const void *data, size_t size)
                    {
                        return Socket::async_write(data, size, boost::fibers::asio::yield);
                    }

                    virtual size_t read(void *data, size_t size)
                    {
                        return Socket::async_read(data, size, boost::fibers::asio::yield);
                    }

                    virtual void close()
                    {
                        Socket::close();
                    }
                }

                template <typename Acceptor>
                class acceptor : public basic_acceptor, private acceptor_t
                {
                public:
                    acceptor(boost::asio::io_context &io_context) : Acceptor(io_context)
                    {
                    }

                    virtual ~acceptor() = default;

                    virtual void bind(const std::string &address)
                    {
                        Acceptor::bind(address);
                    }

                    virtual std::unique_ptr<basic_socket> accept()
                    {
                        std::unique_ptr<basic_socket> ptr = std::make_unique<socket_t>(get_executor().context());
                        Acceptor::accept(static_cast<socket_t&>(*ptr));

                        return ptr;
                    }

                    virtual close()
                    {
                        Acceptor::close();
                    }
                };
            };
        } // namespace protocol
    }     // namespace mq
