#pragma once

#include "parallelism/detail/ring_buffer.hpp"

#include <stddef.h>

#include <atomic>
#include <thread>

namespace mio
{
    namespace parallelism
    {
        template <typename T_, size_t SIZE_ = 4096>
        class ring_queue
        {
        private:
            detail::ring_buffer<T_, SIZE_> buffer_;

            //可读可写标志
            detail::ring_buffer<std::atomic<size_t>, SIZE_> readable_flag_;
            detail::ring_buffer<std::atomic<size_t>, SIZE_> writable_flag_;

            //可读可写界限
            alignas(64) std::atomic<size_t> readable_limit_ = 0;
            alignas(64) std::atomic<size_t> writable_limit_ = 0;

        public:
            struct context
            {
                bool flag = true;
                size_t index = 0;
            };

            ring_queue()
            {
                for (size_t i = 0; i < SIZE_; i++)
                {
                    readable_flag_[i] = 0;
                    writable_flag_[i] = 0;
                }
            }

            void push(const T_ &val)
            {
                size_t index = this->writable_limit_.fetch_add(1);

                //等待可写
                while (writable_flag_[index] != index / SIZE_)
                    std::this_thread::yield();

                this->buffer_[index] = val;
                readable_flag_[index] = (index / SIZE_) + 1;
            }

            bool try_push(const T_ &val, context &context)
            {
                bool &flag = context.flag;

                size_t &index = context.index;

                if (flag == true)
                {
                    index = this->writable_limit_.fetch_add(1);
                }

                //等待可写
                if (writable_flag_[index] != index / SIZE_)
                {
                    flag = false;
                    return false;
                }

                this->buffer_[index] = val;
                readable_flag_[index] = (index / SIZE_) + 1;

                flag = true;
                return true;
            }

            void pop(T_ &val)
            {
                size_t index = this->readable_limit_.fetch_add(1);

                //等待可读
                while (readable_flag_[index] != (index / SIZE_) + 1)
                    std::this_thread::yield();

                val = this->buffer_[index];
                writable_flag_[index] = (index / SIZE_) + 1;
            }

            bool try_pop(T_ &val, context &context)
            {
                bool &flag = context.flag;

                size_t &index = context.index;

                if (flag == true)
                {
                    index = this->readable_limit_.fetch_add(1);
                }

                //等待可读
                if (readable_flag_[index] != (index / SIZE_) + 1)
                {
                    flag = false;
                    return false;
                }

                val = this->buffer_[index];
                writable_flag_[index] = (index / SIZE_) + 1;

                flag = true;
                return true;
            }
        };
    } // namespace parallelism
} // namespace mio