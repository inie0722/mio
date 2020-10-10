#pragma once

#include <stddef.h>
#include <string.h>

#include <atomic>
#include <thread>

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>

namespace mio
{
    namespace parallelism
    {
        template <size_t SIZE_ = 4096>
        class pipe
        {
        private:
            alignas(64) char buffer_[SIZE_];

            alignas(64) std::atomic<size_t> readable_limit_;
            alignas(64) std::atomic<size_t> writable_limit_;

            size_t get_index(size_t index)
            {
                return index % SIZE_;
            }

        public:
            pipe() : readable_limit_(0), writable_limit_(0){};

            pipe(const pipe &) = delete;
            pipe(const pipe &&) = delete;
            pipe &operator=(const pipe &) = delete;

            void send(const void *data, size_t size)
            {
                //等待可写
                while (readable_limit_ + SIZE_ < writable_limit_ + size)
                    std::this_thread::yield();

                if ((writable_limit_ % SIZE_) + size > SIZE_)
                {
                    auto len = SIZE_ - (writable_limit_ % SIZE_);
                    memcpy(&this->buffer_[get_index(writable_limit_)], data, len);
                    memcpy(&this->buffer_[get_index(writable_limit_ + len)], (char *)data + len, size - len);
                }
                else
                {
                    memcpy(&this->buffer_[get_index(writable_limit_)], data, size);
                }
                writable_limit_ += size;
            }

            bool try_send(const void *data, size_t size)
            {
                //等待可写
                if (readable_limit_ + SIZE_ < writable_limit_ + size)
                    return false;

                if ((writable_limit_ % SIZE_) + size > SIZE_)
                {
                    auto len = SIZE_ - (writable_limit_ % SIZE_);
                    memcpy(&this->buffer_[get_index(writable_limit_)], data, len);
                    memcpy(&this->buffer_[get_index(writable_limit_ + len)], (char *)data + len, size - len);
                }
                else
                    memcpy(&this->buffer_[get_index(writable_limit_)], data, size);
                writable_limit_ += size;
                return true;
            }

            void recv(void *data, size_t size)
            {
                //等待可读
                while (readable_limit_ + size > writable_limit_)
                    std::this_thread::yield();

                if ((readable_limit_ % SIZE_) + size > SIZE_)
                {
                    auto len = SIZE_ - (readable_limit_ % SIZE_);
                    memcpy(data, &this->buffer_[get_index(readable_limit_)], len);
                    memcpy((char *)data + len, &this->buffer_[get_index(readable_limit_ + len)], size - len);
                }
                else
                {
                    memcpy(data, &this->buffer_[get_index(readable_limit_)], size);
                }
                readable_limit_ += size;
            }

            bool try_recv(void *data, size_t size)
            {
                //等待可读
                if (readable_limit_ + size > writable_limit_)
                    return false;

                if ((readable_limit_ % SIZE_) + size > SIZE_)
                {
                    auto len = SIZE_ - (readable_limit_ % SIZE_);
                    memcpy(data, &this->buffer_[get_index(readable_limit_)], len);
                    memcpy((char *)data + len, &this->buffer_[get_index(readable_limit_ + len)], size - len);
                }
                else
                    memcpy(data, &this->buffer_[get_index(readable_limit_)], size);
                readable_limit_ += size;
                return true;
            }

            void async_send(const void *data, size_t size, boost::asio::yield_context yield)
            {
                while (!this->try_send(data, size))
                {
                    boost::asio::post(yield);
                }
            }

            void async_recv(void *data, size_t size, boost::asio::yield_context yield)
            {
                while (!this->try_recv(data, size))
                {
                    boost::asio::post(yield);
                }
            }

            size_t size() const
            {
                return writable_limit_ - readable_limit_;
            }
        };
    } // namespace parallelism
} // namespace mio