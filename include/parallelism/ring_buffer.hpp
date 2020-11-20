#pragma once

#include "parallelism/detail/ring_buffer.hpp"

#include <stddef.h>

#include <atomic>
#include <vector>

namespace mio
{
    namespace parallelism
    {
        template <typename T_, typename Container_ = std::vector<T_>>
        class ring_buffer : public detail::ring_buffer<T_, Container_>
        {
        private:
            typedef detail::ring_buffer<T_, Container_> base_t;
            //可写界限
            alignas(64) std::atomic<size_t> writable_limit_ = 0;

        public:
            ring_buffer()
            {
            }

            template <typename... Args>
            ring_buffer(Args &&... args) : base_t(std::forward<Args...>(args)...)
            {
            }

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
                static_cast<base_t &>(*this) = other;
                this->writable_limit_ = other.writable_limit_.load();
                return *this;
            }

            ring_buffer &operator=(ring_buffer &&other)
            {
                static_cast<base_t &>(*this) = other;
                this->writable_limit_ = other.writable_limit_.load();
                return *this;
            }

            using base_t::operator[];

            void push(const T_ &val)
            {
                size_t index = this->writable_limit_;
                (*this)[index] = val;
                ++this->writable_limit_;
            }

            void push(T_ &&val)
            {
                size_t index = this->writable_limit_;
                (*this)[index] = std::move(val);
                ++this->writable_limit_;
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