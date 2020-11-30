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

                std::unique_ptr<boost::interprocess::managed_shared_memory> shared_memory_;
                parallelism::ring_queue<detail::request> *request_queue_ = nullptr;

            public:
                acceptor(boost::asio::io_context &io_context) : io_context_(io_context)
                {
                }

                void bind(const std::string &address)
                {
                    using namespace boost::interprocess;

                    address_ = address;
                    shared_memory_object::remove(address_.c_str());
                    shared_memory_ = std::make_unique<managed_shared_memory>(create_only, address.c_str(), 128 * 1024 * 1024);

                    request_queue_ = shared_memory_->construct<std::remove_reference<decltype(*request_queue_)>::type>("request_queue")();
                }

                void accept(socket &peer)
                {
                    detail::request req;
                    request_queue_->pop(req);

                    new (&peer) socket(io_context_, req.channel_ptr.get());
                }

                void accept(socket &peer, boost::asio::yield_context yield)
                {
                    detail::request req;
                    request_queue_->pop(req, [&](size_t) { this->io_context_.post(yield); });

                    new (&peer) socket(io_context_, req.channel_ptr.get());
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
            };
        } // namespace pipe
    }     // namespace interprocess
} // namespace mio