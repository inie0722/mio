#pragma once

#include <cstddef>
#include <atomic>
#include <tuple>
#include <list>
#include <mutex>
#include <semaphore>

#include <fmt/format.h>

#include <mio/chrono.hpp>

namespace mio
{
    template <size_t N = 65536>
    class log
    {
    private:
        struct message
        {
            void (*fun)(void *ptr);
            void *ptr;
            std::size_t size;
        };

        class spsc_buffer
        {
        private:
            char data_[N];
            std::atomic<std::size_t> writable_limit_ = 0;
            std::atomic<std::size_t> readable_limit_ = 0;

        public:
            message *alloc(std::size_t size)
            {
                auto all_size = size + sizeof(message);

                auto writable_limit = writable_limit_.load();
                auto readable_limit = readable_limit_.load();
                auto r = N - writable_limit % N;

                message *ret = reinterpret_cast<message *>(&data_[writable_limit % N]);
                all_size = r < all_size + sizeof(message) ? r + all_size + sizeof(message) : all_size;
                void *ptr = r < all_size ? static_cast<void *>(&data_[0]) : static_cast<void *>(&ret[1]);

                //写满了等待 可写
                while (N - (writable_limit - readable_limit) < all_size)
                {
                    readable_limit_.wait(readable_limit);
                    readable_limit = readable_limit_.load();
                }

                ret->size = all_size;
                ret->ptr = ptr;
                return ret;
            }

            size_t size() const
            {
                return writable_limit_ - readable_limit_;
            }

            void push(message *msg)
            {
                writable_limit_ += msg->size;
            }

            message *front()
            {
                return (message *)&data_[readable_limit_ % N];
            }

            void pop(message *msg)
            {
                this->readable_limit_ += msg->size;
                this->readable_limit_.notify_one();
            }
        };

        inline static std::mutex mutex_;
        inline static std::list<spsc_buffer *> list_;
        inline static std::counting_semaphore semaphore_{0};
        inline static std::atomic<bool> is_run_ = false;
        inline static std::atomic<std::size_t> count_ = 0;

        spsc_buffer buffer_;
        typename std::list<spsc_buffer *>::iterator iterator_;

        template <typename Stream, typename Format, size_t... Index, typename... Args>
        void operator()(Stream &stream, Format &&fmt, std::index_sequence<Index...>, Args &&...args)
        {
            using args_t = std::tuple<Stream &, std::remove_reference_t<Format>, std::remove_reference_t<Args>...>;

            auto msg = buffer_.alloc(sizeof(args_t));
            new (msg->ptr) args_t(stream, std::forward<Format>(fmt), std::forward<Args>(args)...);
            msg->fun = [](void *ptr)
            {
                auto *args = reinterpret_cast<args_t *>(ptr);
                std::get<size_t>(*args);
                std::get<0>(*args) << fmt::vformat(std::get<1>(*args), fmt::make_format_args(std::get<Index + 2>(*args)...));
                args->~args_t();
            };

            buffer_.push(msg);
            ++count_;
            semaphore_.release();
        }

    public:
        log()
        {
            std::lock_guard lock(mutex_);
            list_.push_back(&buffer_);
            iterator_ = --list_.end();
        }

        ~log()
        {
            std::lock_guard lock(mutex_);
            list_.erase(iterator_);
        }

        template <typename Stream, typename Format, typename... Args, typename Index = std::make_index_sequence<sizeof...(Args)>>
        void operator()(Stream &stream, Format &&fmt, Args &&...args)
        {
            this->operator()(stream, std::forward<Format>(fmt), Index{}, std::forward<Args>(args)...);
        }

        size_t size() const
        {
            return buffer_.size();
        }

        static void run()
        {
            is_run_ = true;
            while (1)
            {
                semaphore_.acquire();

                if (!is_run_)
                    return;

                std::lock_guard lock(mutex_);
                for (size_t i = 0; i < list_.size(); i++)
                {
                    auto buffer = list_.front();
                    list_.pop_front();
                    if (buffer->size())
                    {
                        auto msg = buffer->front();
                        msg->fun(msg->ptr);
                        buffer->pop(msg);
                        --count_;
                        list_.push_back(buffer);
                        break;
                    }
                    else
                    {
                        list_.push_back(buffer);
                    }
                }
            }
        }

        static void stop()
        {
            is_run_ = false;
            semaphore_.release();
        }
    };
}