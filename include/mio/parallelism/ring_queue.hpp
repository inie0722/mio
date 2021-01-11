#pragma once

#include "mio/parallelism/detail/ring_buffer.hpp"
#include "mio/parallelism/utility.hpp"

#include <stddef.h>

#include <atomic>
#include <array>
#include <utility>
#include <algorithm>

namespace mio
{
    namespace parallelism
    {
        template <typename T, typename Allocator = std::allocator<T>>
        class ring_queue
        {
        private:
            using allocator_traits = std::allocator_traits<Allocator>;

        public:
            using value_type = typename allocator_traits::value_type;

            using allocator_type = typename allocator_traits::allocator_type;

            using size_type = typename allocator_traits::size_type;

            using reference = typename allocator_traits::value_type &;

            using const_reference = const typename allocator_traits::value_type &;

            using pointer = typename allocator_traits::pointer;

            using const_pointer = typename allocator_traits::const_pointer;

        private:
            size_type max_size_;
            detail::ring_buffer<value_type, Allocator> buffer_;

            //可读可写标志
            detail::ring_buffer<std::atomic<size_type>, Allocator> readable_flag_;
            detail::ring_buffer<std::atomic<size_type>, Allocator> writable_flag_;

            //可读可写界限
            alignas(CACHE_LINE) std::atomic<size_type> readable_limit_;
            alignas(CACHE_LINE) std::atomic<size_type> writable_limit_;

        public:
            ring_queue(size_type max_size)
            :max_size_(max_size), buffer_(max_size), readable_flag_(max_size), writable_flag_(max_size), readable_limit_(0), writable_limit_(0)
            {
                for (size_t i = 0; i < max_size; i++)
                {
                    readable_flag_[i] = 0;
                    writable_flag_[i] = 0;
                }
            }

            void push(const T &val, const wait::handler_t &handler = wait::yield)
            {
                size_t index = this->writable_limit_.fetch_add(1);

                //等待可写
                for (size_t i = 0; writable_flag_[index] != index / this->max_size(); i++)
                    handler(i);

                this->buffer_[index] = val;
                readable_flag_[index] = (index / this->max_size()) + 1;
            }

            void push(T &&val, const wait::handler_t &handler = wait::yield)
            {
                size_t index = this->writable_limit_.fetch_add(1);

                //等待可写
                for (size_t i = 0; writable_flag_[index] != index / this->max_size(); i++)
                    handler(i);

                this->buffer_[index] = std::move(val);
                readable_flag_[index] = (index / this->max_size()) + 1;
            }

            void pop(T &val, const wait::handler_t &handler = wait::yield)
            {
                size_t index = this->readable_limit_.fetch_add(1);

                //等待可读
                for (size_t i = 0; readable_flag_[index] != (index / this->max_size()) + 1; i++)
                    handler(i);

                val = std::move(this->buffer_[index]);
                writable_flag_[index] = (index / this->max_size()) + 1;
            }

            size_t size() const
            {
                size_t writable_limit = writable_limit_;
                size_t readable_limit = readable_limit_;

                return writable_limit > readable_limit ? writable_limit - readable_limit : 0;
            }

            size_t max_size() const
            {
                return this->max_size_;
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