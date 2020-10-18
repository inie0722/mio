#pragma once

#include "parallelism/detail/ring_buffer.hpp"

#include <stddef.h>

#include <atomic>

namespace mio
{
    namespace parallelism
    {
        template<typename T_, size_t SIZE_ = 4096>
        class ring_buffer
        {
        private:
            detail::ring_buffer<T_, SIZE_> buffer_;

            //可写界限
            alignas(64) std::atomic<size_t> writable_limit_ = 0;

        public:
            ring_buffer()
            {
            }

            void push(const T_ & val)
            {
                size_t index = this->writable_limit_;
                this->buffer_[index] = val;
                this->writable_limit_.fetch_add(1);
            }

            T & operator[](size_t index)
            {
                return buffer_[index];
            }

            size_t size() const
            {
                return writable_limit_.load();
            }
        };
    }
}