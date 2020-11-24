#pragma once

#include <string>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/asio.hpp>

#include <memory>

#include "parallelism/ring_queue.hpp"
#include "parallelism/channel.hpp"

#include "interprocess/acceptor.hpp"
#include "interprocess/detail.hpp"

#include <iostream>

namespace mio
{
    namespace interprocess
    {
        class socket
        {
        private:
            std::string address_;
            boost::asio::io_context &io_context_;

            std::unique_ptr<boost::interprocess::managed_shared_memory> shared_memory_;
            parallelism::channel<char>* channel_;
            int is_clinet;
            parallelism::ring_queue<detail::request> *request_queue_;

        public:
            socket(decltype(io_context_) &io_context, const decltype(channel_) & channel):io_context_(io_context), channel_(channel)
            {
                is_clinet = 0;
            }

            socket(decltype(io_context_) &io_context):io_context_(io_context)
            {
                is_clinet = 1;
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

            size_t write(const char *data, size_t size)
            {
                channel_->write(is_clinet, data, size);
                return size;
            }

            size_t read(char *data, size_t size)
            {
                channel_->read(is_clinet, data, size);
                return size;
            }
            
            void close()
            {
                
            }

            ~socket()
            {
            }
        };
    } // namespace interprocess
} // namespace mio