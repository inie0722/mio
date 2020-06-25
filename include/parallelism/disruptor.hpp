#pragma once

#include "ring_buffer.hpp"

#include <assert.h>
#include <stdint.h>
#include <atomic>
#include <memory>
#include <utility>
#include <thread>

namespace libcpp
{
    template <typename T, size_t READER_NUM, size_t SIZE = 4096>
    class disruptor
    {
    public:
        class alignas(64) reader
        {
        private:
            friend disruptor;
            disruptor *queue;

            std::atomic<size_t> limit = 0;
            reader(){};
            reader(disruptor *queue) : queue(queue)
            {
                this->limit = this->queue->writable_limit.load();
            }

        public:
            void pull(T &ret)
            {
                //等待数据可读
                while (queue->readable_flag[this->limit].load() != (limit / SIZE) + 1)
                    ;

                ret = queue->buffer[this->limit.load()];
                limit.fetch_add(1);
            }

            bool try_pull(T &ret)
            {
                //等待数据可读
                while (queue->readable_flag[this->limit].load() != (limit / SIZE) + 1)
                    return false;

                ret = queue->buffer[this->limit.load()];
                limit.fetch_add(1);
                return true;
            }
        };

    private:
        friend reader;

        detail::ring_buffer<T, SIZE> buffer;
        detail::ring_buffer<std::atomic<uint64_t>, SIZE> readable_flag;

        alignas(64) std::atomic<uint64_t> writable_limit = 0;
        alignas(64) std::atomic<uint64_t> reader_count = 0;

        reader reader_list[READER_NUM];

        bool min_index(size_t index)
        {
            for (size_t i = 0; i < this->reader_count.load(); i++)
            {
                if (index >= this->reader_list[i].limit.load() + SIZE)
                    return false;
            }
            return true;
        }

    public:
        disruptor()
        {
        }

        reader *make_reader()
        {
            assert(this->reader_count.load() != READER_NUM);
            auto index = this->reader_count.fetch_add(1);

            return new (&reader_list[index]) reader(this);
        }

        void push(const T &val)
        {
            size_t index = this->writable_limit.fetch_add(1);

            //等待可写
            while (!this->min_index(index))
                std::this_thread::yield();

            this->buffer[index] = val;
            this->readable_flag[index].store((index / SIZE) + 1);
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
            if (!this->min_index(index))
            {
                flag = false;
                return false;
            }

            this->buffer[index] = val;
            this->readable_flag[index].store((index / SIZE) + 1);

            flag = true;
            return true;
        }
    };
} // namespace libcpp
