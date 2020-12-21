#pragma once

#include "parallelism/detail/ring_buffer.hpp"

#include <stdint.h>

#include <atomic>
#include <thread>
#include <list>
#include <shared_mutex>

namespace mio
{
    namespace parallelism
    {
        template <typename T_, size_t SIZE_ = 4096>
        class disruptor
        {
        public:
            class alignas(64) reader
            {
            private:
                friend disruptor;
                disruptor *disruptor_;
                typename std::list<reader *>::iterator this_it_;

                std::atomic<size_t> limit_ = 0;

                reader(disruptor *disruptor, typename std::list<reader *>::iterator &this_it) : disruptor_(disruptor), this_it_(this_it)
                {
                    this->limit_ = this->disruptor_->writable_limit_.load();
                }

            public:
                void pull(T_ &ret)
                {
                    //等待数据可读
                    while (disruptor_->readable_flag_[this->limit_] != (limit_ / SIZE_) + 1)
                        std::this_thread::yield();

                    ret = disruptor_->buffer_[this->limit_];
                    limit_.fetch_add(1);
                }

                bool try_pull(T_ &ret)
                {
                    //等待数据可读
                    while (disruptor_->readable_flag_[this->limit_] != (limit_ / SIZE_) + 1)
                        return false;

                    ret = disruptor_->buffer_[this->limit_];
                    limit_.fetch_add(1);
                    return true;
                }

                ~reader()
                {
                    disruptor_->mutex_.lock();
                    disruptor_->reader_list_.erase(this_it_);
                    disruptor_->mutex_.unlock();
                }
            };

        private:
            friend reader;

            detail::ring_buffer<T_, SIZE_> buffer_;
            detail::ring_buffer<std::atomic<size_t>, SIZE_> readable_flag_;

            alignas(64) std::atomic<size_t> writable_limit_ = 0;

            std::shared_mutex mutex_;
            std::list<reader *> reader_list_;

            bool min_index(size_t index)
            {
                this->mutex_.lock_shared();
                for (auto &it : this->reader_list_)
                {
                    if (index >= it->limit_ + SIZE_)
                    {
                        this->mutex_.unlock_shared();
                        return false;
                    }
                }

                this->mutex_.unlock_shared();
                return true;
            }

        public:
            struct context
            {
                bool flag = true;
                size_t index = 0;
            };

            disruptor()
            {
                for (size_t i = 0; i < SIZE_; i++)
                {
                    readable_flag_[i] = 0;
                }
            }

            reader *make_reader()
            {
                this->mutex_.lock();
                this->reader_list_.push_back(NULL);
                this->reader_list_.back() = new reader(this, --this->reader_list_.end());
                this->mutex_.unlock();
                return this->reader_list_.back();
            }

            void push(const T_ &val)
            {
                size_t index = this->writable_limit_.fetch_add(1);

                //等待可写
                while (!this->min_index(index))
                    std::this_thread::yield();

                this->buffer_[index] = val;
                this->readable_flag_[index] = (index / SIZE_) + 1;
            }

            bool try_push(const T_ &val, context &context)
            {
                bool &flag = context.flag;

                size_t &index = context.index;

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
                this->readable_flag[index] = (index / SIZE_) + 1;

                flag = true;
                return true;
            }
        };
    } // namespace parallelism
} // namespace mio
