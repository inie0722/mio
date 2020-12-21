#pragma once

#include <string>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/asio.hpp>

#include <memory>
#include <type_traits>

#include "parallelism/ring_queue.hpp"
#include "interprocess/pipe/socket.hpp"
#include "interprocess/pipe/detail.hpp"

namespace mio
{
    namespace interprocess
    {
        namespace pipe
        {
            class acceptor
            {
            private:
                std::string address_;
                boost::asio::io_context &io_context_;

                std::shared_ptr<boost::interprocess::managed_shared_memory> shared_memory_;
                parallelism::ring_queue<detail::request> *request_queue_ = nullptr;

            public:
                acceptor(boost::asio::io_context &io_context) : io_context_(io_context)
                {
                }

                acceptor(acceptor &&other) : address_(std::move(other.address_)), io_context_(other.io_context_), shared_memory_(std::move(other.shared_memory_))
                {
                    request_queue_ = other.request_queue_;
                }

                void bind(const std::string &address)
                {
                    using namespace boost::interprocess;

                    address_ = address;
                    shared_memory_object::remove(address_.c_str());
                    shared_memory_ = std::make_shared<managed_shared_memory>(create_only, address.c_str(), 128 * 1024 * 1024);

                    request_queue_ = shared_memory_->construct<std::remove_reference<decltype(*request_queue_)>::type>("request_queue")();
                }

                void accept(socket &peer)
                {
                    detail::request req;
                    request_queue_->pop(req);

                    new (&peer) socket(io_context_, shared_memory_, req.channel_ptr.get());
                }

                template <typename Yield>
                void async_accept(socket &peer, Yield yield)
                {
                    detail::request req;
                    request_queue_->pop(req, [&](size_t) { this->io_context_.post(yield); });

                    new (&peer) socket(io_context_, shared_memory_, req.channel_ptr.get());
                }

                void close()
                {
                    using namespace boost::interprocess;

                    if (request_queue_ != nullptr)
                    {
                        shared_memory_object::remove(address_.c_str());
                    }
                }

                ~acceptor()
                {
                    close();
                }

                auto get_executor()
                {
                    return io_context_.get_executor();
                }
            };
        } // namespace pipe
    }     // namespace interprocess
} // namespace mio