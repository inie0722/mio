#pragma once

#include <stddef.h>

#include <atomic>
#include <vector>

namespace mio
{
    namespace parallelism
    {
        template <typename T_, typename Container_ = std::vector<T_>>
        class ring_buffer
        {
        public:
            typedef Container_ container_type;

            typedef typename container_type::value_type value_type;
            typedef typename container_type::size_type size_type;
            typedef typename container_type::reference reference;
            typedef typename container_type::const_reference const_reference;

        protected:
            alignas(64) container_type c;

        private:
            //可写界限
            alignas(64) std::atomic<size_t> writable_limit_ = 0;

        public:
            ring_buffer()
            {
            }

            template <typename... Args>
            ring_buffer(Args &&... args) : c(std::forward<Args...>(args)...)
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
                this->c = other.c;
                this->writable_limit_ = other.writable_limit_.load();
            }

            ring_buffer &operator=(ring_buffer &&other)
            {
                this->c = std::move(other.c);
                this->writable_limit_ = other.writable_limit_.load();
            }

            T_ &operator[](size_t index)
            {
                return c[writable_limit_ - index];
            }

            const T_ &operator[](size_t index) const
            {
                return c[writable_limit_ - index];
            }

            container_type &get_container()
            {
                return this->c;
            }

            const container_type &get_container() const
            {
                return this->c;
            }

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
                return !c.empty();
            }

            bool is_lock_free() const
            {
                return true;
            }
        };
    } // namespace parallelism
} // namespace mio