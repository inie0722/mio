#pragma once

#include <string>
#include <memory>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/asio.hpp>

#include "mio/parallelism/ring_queue.hpp"
#include "mio/parallelism/channel.hpp"

#include "mio/interprocess/pipe/acceptor.hpp"
#include "mio/interprocess/pipe/detail.hpp"

namespace mio
{
    namespace interprocess
    {
        namespace pipe
        {
            class socket
            {
            private:
                std::string address_;
                boost::asio::io_context &io_context_;

                std::shared_ptr<boost::interprocess::managed_shared_memory> shared_memory_;
                parallelism::channel<char> *channel_ = nullptr;

                parallelism::ring_queue<detail::request> *request_queue_;

                bool is_clinet_;

            public:
                socket(decltype(io_context_) &io_context, const decltype(shared_memory_) & shared_memory, const decltype(channel_) &channel) : io_context_(io_context), shared_memory_(shared_memory), channel_(channel)
                {
                    is_clinet_ = 0;
                }

                socket(decltype(io_context_) &io_context) : io_context_(io_context)
                {
                    is_clinet_ = 1;
                }

                socket(socket &&other) : io_context_(other.io_context_), shared_memory_(std::move(other.shared_memory_))
                {

                    channel_ = other.channel_;
                    request_queue_ = other.request_queue_;
                    is_clinet_ = other.is_clinet_;

                    other.channel_ = nullptr;
                }

                void connect(const std::string &address)
                {
                    using namespace boost::interprocess;

                    address_ = address;
                    shared_memory_ = std::make_unique<managed_shared_memory>(open_only, address.c_str());
                    request_queue_ = shared_memory_->find<std::remove_reference<decltype(*request_queue_)>::type>("request_queue").first;
                    channel_ = shared_memory_->construct<std::remove_reference<decltype(*channel_)>::type>(anonymous_instance)();

                    detail::request req;
                    req.channel_ptr = channel_;
                    request_queue_->push(req);
                }

                template <typename Yield>
                void async_connect(const std::string &address, Yield &yield)
                {
                    using namespace boost::interprocess;

                    address_ = address;
                    shared_memory_ = std::make_unique<managed_shared_memory>(open_only, address.c_str());
                    request_queue_ = shared_memory_->find<std::remove_reference<decltype(*request_queue_)>::type>("request_queue").first;
                    channel_ = shared_memory_->construct<std::remove_reference<decltype(*channel_)>::type>(anonymous_instance)();

                    detail::request req;
                    req.channel_ptr = channel_;
                    request_queue_->push(req, [&](size_t) { this->io_context_.post(yield); });
                }

                size_t write_some(const void *data, size_t size)
                {
                    return channel_->write_some(is_clinet_, (char *)data, size);
                }

                size_t read_some(void *data, size_t size)
                {
                    return channel_->read_some(is_clinet_, (char *)data, size);
                }

                template <typename Yield>
                size_t async_write_some(const void *data, size_t size, Yield &yield)
                {
                    return channel_->write_some(is_clinet_, (char *)data, size, [&](size_t) { this->io_context_.post(yield); });
                }

                template <typename Yield>
                size_t async_read_some(void *data, size_t size, Yield &yield)
                {
                    return channel_->read_some(is_clinet_, (char *)data, size, [&](size_t) { this->io_context_.post(yield); });
                }

                size_t write(const void *data, size_t size)
                {
                    return channel_->write(is_clinet_, (char *)data, size);
                }

                size_t read(void *data, size_t size)
                {
                    return channel_->read(is_clinet_, (char *)data, size);
                }

                template <typename Yield>
                size_t async_write(const void *data, size_t size, Yield &yield)
                {
                    return channel_->write(is_clinet_, (char *)data, size, [&](size_t) { this->io_context_.post(yield); });
                }

                template <typename Yield>
                size_t async_read(void *data, size_t size, Yield &yield)
                {
                    return channel_->read(is_clinet_, (char *)data, size, [&](size_t) { this->io_context_.post(yield); });
                }

                void close()
                {
                     using namespace boost::interprocess;
                     
                    if (channel_ != nullptr)
                    {
                        if (channel_->get_status() == parallelism::channel<char>::DISCONNECTED)
                        {
                            shared_memory_->destroy_ptr(channel_);
                        }
                        channel_->close();

                        shared_memory_object::remove(address_.c_str());
                        channel_ = nullptr;
                    }
                }

                ~socket()
                {
                    close();
                }

                bool is_open() const
                {
                    return channel_;
                }

                auto get_executor()
                {
                    return io_context_.get_executor();
                }
            };
        } // namespace pipe
    }     // namespace interprocess
} // namespace mio