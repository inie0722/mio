/**
 * @file ring_queue.hpp
 * @author 然Y (inie0722@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2022-03-07
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#pragma once

#include <atomic>
#include <cstddef>
#include <memory>

namespace mio
{
    namespace parallelism
    {
        /**
         * @brief 多写多读 无锁有界队列
         * @details 无等待数据结构
         * @tparam T 
         * @tparam Allocator 
         */
        template <typename T, typename Allocator = std::allocator<T>>
        class ring_queue
        {
        public:
            using value_type = T;
            using size_type = std::size_t;

        private:
            struct node
            {
                std::atomic<size_type> readable_flag;
                std::atomic<size_type> writable_flag;
                value_type value;
            };

            using allocator_type = typename std::allocator_traits<Allocator>::rebind_alloc<node>;
            using allocator_traits = std::allocator_traits<allocator_type>;
            using pointer = allocator_traits::pointer;

            allocator_type alloc_;
            pointer data_;
            size_type max_size_;

            //可读可写界限
            std::atomic<size_type> readable_limit_ = 0;
            std::atomic<size_type> writable_limit_ = 0;

            size_type get_index(size_type index) const
            {
                return index % max_size_;
            }

        public:
            ring_queue(size_type max_size)
                : max_size_(max_size)
            {
                this->data_ = this->alloc_.allocate(max_size);
                for (size_t i = 0; i < max_size; i++)
                {
                    allocator_traits::construct(this->alloc_, &this->data_[i], 0, i);
                }
            }

            ~ring_queue()
            {
                for (size_t i = 0; i < this->max_size_; i++)
                {
                    allocator_traits::destroy(this->alloc_, this->data_);
                }
                this->alloc_.deallocate(this->data_, this->max_size_);
            }

            void push(const T &val)
            {
                size_t index = this->writable_limit_.fetch_add(1);

                while (1)
                {
                    size_type writable_flag = this->data_[get_index(index)].writable_flag;

                    if (writable_flag != index)
                        this->data_[get_index(index)].writable_flag.wait(writable_flag);
                    else
                        break;
                }

                this->data_[get_index(index)].value = val;
                this->data_[get_index(index)].readable_flag = index + 1;
                this->data_[get_index(index)].readable_flag.notify_one();
            }

            void pop(T &val)
            {
                size_t index = this->readable_limit_.fetch_add(1);

                while (1)
                {
                    size_type readable_flag = this->data_[get_index(index)].readable_flag;

                    if (readable_flag != index + 1)
                        this->data_[get_index(index)].readable_flag.wait(readable_flag);
                    else
                        break;
                }

                val = std::move(this->data_[get_index(index)].value);
                this->data_[get_index(index)].writable_flag = index + this->max_size();
                this->data_[get_index(index)].writable_flag.notify_one();
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
                return readable_limit_.is_lock_free();
            }
        };
    } // namespace parallelism
} // namespace mio
