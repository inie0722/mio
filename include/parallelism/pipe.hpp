#pragma once

#include <stddef.h>
#include <string.h>

#include <atomic>
#include <thread>

namespace mio
{
    namespace parallelism
    {
        template <size_t SIZE_ = 4096>
        class pipe
        {
        private:
            alignas(64) char buffer_[SIZE_];

            alignas(64) std::atomic<size_t> readable_limit_ = 0;
            alignas(64) std::atomic<size_t> writable_limit_ = 0;

            size_t get_index(size_t index)
            {
                return index % SIZE_;
            }

            void __sned(const void *data, size_t size)
            {
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

            void __recv(void *data, size_t size)
            {
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

            bool is_can_send(size_t size)
            {
                return !(readable_limit_ + SIZE_ < writable_limit_ + size);
            }

            bool is_can_recv(size_t size)
            {
                return !(readable_limit_ + size > writable_limit_);
            }

        public:
            pipe()
            {
            }

            pipe(const pipe &) = delete;
            pipe(const pipe &&) = delete;
            pipe &operator=(const pipe &) = delete;

            void send(const void *data, size_t size)
            {
                //等待可写
                while (!is_can_send(size))
                    std::this_thread::yield();

                __sned(data, size);
            }

            bool try_send(const void *data, size_t size)
            {
                //等待可写
                if (!is_can_send(size))
                    return false;

                __sned(data, size);
                return true;
            }

            void recv(void *data, size_t size)
            {
                //等待可读
                while (!is_can_recv(size))
                    std::this_thread::yield();

                __recv(data, size);
            }

            bool try_recv(void *data, size_t size)
            {
                if (!is_can_recv(size))
                    return false;

                __recv(data, size);
                return true;
            }

            size_t size() const
            {
                return writable_limit_ - readable_limit_;
            }
        };
    } // namespace parallelism
} // namespace mio