#pragma once

#include "parallelism/detail/ring_buffer.hpp"

#include <stddef.h>

#include <atomic>
#include <array>
#include <utility>
#include <algorithm>

namespace mio
{
    namespace parallelism
    {
        template <typename T_, size_t N_>
        class ring_buffer : public detail::ring_buffer<T_, Container_>
        {
        private:
            detail::ring_buffer<T_, N_> c;
            //可写界限
            alignas(CACHE_LINE) std::atomic<size_t> writable_limit_ = 0;

        public:
            ring_buffer() = default;

            ring_buffer(const ring_buffer &other)
            {
                *this = other;
            }

            ring_buffer(ring_buffer &&other)
            {
                *this = other;
            }

            ring_buffer &operator=(const ring_buffer &other)
            {
                c = other.c;
                writable_limit_ = other.writable_limit_.load();
                return *this;
            }

            ring_buffer &operator=(ring_buffer &&other)
            {
                c = std::move(other.c);
                writable_limit_ = other.writable_limit_.load();
                return *this;
            }

            void push(const T_ &val)
            {
                size_t index = this->writable_limit_;
                c.[index] = val;
                ++this->writable_limit_;
            }

            void push(T_ &&val)
            {
                size_t index = this->writable_limit_;
                c.[index] = std::move(val);
                ++this->writable_limit_;
            }

            T_ &operator[](size_t index)
            {
                return this->c[writable_limit_ - 1 - index];
            }

            const T_ &operator[](size_t index) const
            {
                return this->c[writable_limit_ - 1 - index];
            }

            size_t size() const
            {
                return writable_limit_;
            }

            bool empty() const
            {
                return !this->size();
            }

            bool is_lock_free() const
            {
                return true;
            }
        };
    } // namespace parallelism
} // namespace mio