#pragma once

#include <stdint.h>
#include <memory>

#include <boost/fiber/buffered_channel.hpp>

#include "mq/message.hpp"

namespace mio
{
    namespace mq
    {
        class future
        {
        private:
            friend class promise;
            size_t size_;

            std::shared_ptr<boost::fibers::buffered_channel<std::unique_ptr<message>>> pipe_;

            future(size_t size, const decltype(pipe_) &pipe) : pipe_(pipe)
            {
                size_ = size;
            }

        public:
            std::unique_ptr<message> get()
            {
                std::unique_ptr<message> msg;
                pipe_->pop(msg);

                return msg;
            }

            size_t size()
            {
                return size_;
            }
        };

        class promise
        {
        private:
            size_t size_;
            std::shared_ptr<boost::fibers::buffered_channel<std::unique_ptr<message>>> pipe_;

        public:
            promise(size_t size)
            {
                size_ = size;
                pipe_ = std::make_shared<boost::fibers::buffered_channel<std::unique_ptr<message>>>(64);
            }

            future get_future()
            {
                return future(size_, pipe_);
            }

            void set_value(std::unique_ptr<message> &&msg)
            {
                pipe_->push(std::move(msg));
            }
        };
    } // namespace mq
} // namespace mio