#pragma once

#include "ring_buffer.hpp"

#include <stddef.h>
#include <stdint.h>

#include <atomic>
#include <thread>

namespace libcpp
{
    template <typename T, size_t SIZE = 4096>
    class ring_queue
    {
    private:
        detail::ring_buffer<T, SIZE> buffer;

        //可读可写标志
        detail::ring_buffer<std::atomic<uint64_t>, SIZE> readable_flag;
        detail::ring_buffer<std::atomic<uint64_t>, SIZE> writable_flag;

        //可读可写界限
        alignas(64) std::atomic<uint64_t> readable_limit = 0;
        alignas(64) std::atomic<uint64_t> writable_limit = 0;

    public:
        void push(const T &val)
        {
            size_t index = this->writable_limit.fetch_add(1);

            //等待可写
            while (writable_flag[index] != index / SIZE)
                std::this_thread::yield();

            this->buffer[index] = val;
            readable_flag[index].store((index / SIZE) + 1);
        }

        bool try_push(const T &val)
        {
            thread_local bool flag = true;

            thread_local size_t index;

            if (flag == true)
            {
                index = this->writable_limit.fetch_add(1);
            }

            //等待可写
            if (writable_flag[index] != index / SIZE)
            {
                flag = false;
                return false;
            }

            this->buffer[index] = val;
            readable_flag[index].store((index / SIZE) + 1);

            flag = true;
            return true;
        }

        void pop(T &val)
        {
            size_t index = this->readable_limit.fetch_add(1);

            //等待可读
            while (readable_flag[index] != (index / SIZE) + 1)
                std::this_thread::yield();

            val = this->buffer[index];
            writable_flag[index].store((index / SIZE) + 1);
        }

        bool try_pop(T &val)
        {
            thread_local bool flag = true;

            thread_local size_t index;

            if (flag == true)
            {
                index = this->readable_limit.fetch_add(1);
            }

            //等待可读
            if (readable_flag[index] != (index / SIZE) + 1)
            {
                flag = false;
                return false;
            }

            val = this->buffer[index];
            writable_flag[index].store((index / SIZE) + 1);

            flag = true;
            return true;
        }
    };
} // namespace libcpp