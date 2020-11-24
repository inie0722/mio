#pragma once

#include "parallelism/detail/ring_buffer.hpp"
#include "parallelism/utility.hpp"

#include <stddef.h>

#include <atomic>
#include <array>
#include <utility>
#include <algorithm>

namespace mio
{
    namespace parallelism
    {
        template <typename T_, size_t N_ = 4096>
        class ring_queue
        {
        private:
            detail::ring_buffer<T_, N_> buffer_;

            //可读可写标志
            detail::ring_buffer<std::atomic<size_t>, N_> readable_flag_;
            detail::ring_buffer<std::atomic<size_t>, N_> writable_flag_;

            //可读可写界限
            alignas(CACHE_LINE) std::atomic<size_t> readable_limit_ = 0;
            alignas(CACHE_LINE) std::atomic<size_t> writable_limit_ = 0;

        public:
            ring_queue()
            {
                for (size_t i = 0; i < N_; i++)
                {
                    readable_flag_[i] = 0;
                    writable_flag_[i] = 0;
                }
            }

            void push(const T_ &val, const wait::handler_t &handler = wait::yield)
            {
                size_t index = this->writable_limit_.fetch_add(1);

                //等待可写
                for (size_t i = 0; writable_flag_[index] != index / N_; i++)
                    handler(i);

                this->buffer_[index] = val;
                readable_flag_[index] = (index / N_) + 1;
            }

            void push(T_ &&val, const wait::handler_t &handler = wait::yield)
            {
                size_t index = this->writable_limit_.fetch_add(1);

                //等待可写
                for (size_t i = 0; writable_flag_[index] != index / N_; i++)
                    handler(i);

                this->buffer_[index] = std::move(val);
                readable_flag_[index] = (index / N_) + 1;
            }

            void pop(T_ &val, const wait::handler_t &handler = wait::yield)
            {
                size_t index = this->readable_limit_.fetch_add(1);

                //等待可读
                for (size_t i = 0; readable_flag_[index] != (index / N_) + 1; i++)
                    handler(i);

                val = std::move(this->buffer_[index]);
                writable_flag_[index] = (index / N_) + 1;
            }
        };
    } // namespace parallelism
} // namespace mio