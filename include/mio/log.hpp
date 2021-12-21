#pragma once

#include <stddef.h>
#include <stdint.h>
#include <atomic>
#include <tuple>
#include <list>
#include <mutex>

#include <boost/interprocess/sync/interprocess_semaphore.hpp>
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
            size_t size;
        };

        class SPSC_buffer
        {
        private:
            char data_[N];
            alignas(128) std::atomic<uint64_t> writable_limit_ = 0;
            alignas(128) std::atomic<uint64_t> readable_limit_ = 0;

        public:
            message *alloc(size_t size)
            {
                auto all_size = size + sizeof(message);

                auto writable_limit = writable_limit_.load();
                auto r = N - writable_limit % N;

                message *ret = (message *)&data_[writable_limit % N];
                all_size = r < all_size + sizeof(message) ? r + all_size + sizeof(message) : all_size;
                void *ptr = r < all_size ? (void*)&data_[0] : (void*)&ret[1];

                //写满了等待 可写
                while (N - (writable_limit - readable_limit_) < all_size)
                    ;

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
            }
        };

        inline static std::mutex mutex_;
        inline static std::list<SPSC_buffer *> list_;
        inline static boost::interprocess::interprocess_semaphore semaphore_{0};
        SPSC_buffer buffer_;
        typename std::list<SPSC_buffer *>::iterator iterator_;

        template <typename Stream, typename Format, size_t... Index, typename... Args>
        void operator()(Stream &stream, const Format &fmt, std::index_sequence<Index...>, const Args &...args)
        {
            using args_t = std::tuple<Stream &, Format, Args...>;

            auto msg = buffer_.alloc(sizeof(args_t));
            new (msg->ptr) args_t(stream, fmt, args...);
            msg->fun = [](void *ptr)
            {
                auto *args = reinterpret_cast<args_t *>(ptr);
                std::get<0>(*args) << fmt::format(std::get<1>(*args), std::get<Index + 2>(*args)...);
            };

            buffer_.push(msg);
            semaphore_.post();
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
        void operator()(Stream &stream, const Format &fmt, const Args &...args)
        {
            this->operator()(stream, fmt, Index{}, args...);
        }

        size_t size() const
        {
            return buffer_.size();
        }

        static void run_once()
        {
            semaphore_.wait();
            std::lock_guard lock(mutex_);
            for (size_t i = 0; i < list_.size(); i++)
            {
                auto front = list_.front();
                list_.pop_front();
                if (front->size())
                {
                    auto msg = front->front();
                    msg->fun(msg->ptr);
                    front->pop(msg);

                    list_.push_back(front);
                    break;
                }
                else
                {
                    list_.push_back(front);
                }
            }
        }
    };
}

template <typename Rep, typename Period>
struct fmt::formatter<std::chrono::duration<Rep, Period>>
{
    constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin())
    {
        auto it = ctx.begin();
        auto end = ctx.end();

        if (it != end && it[0] == 't' && it[1] == 'm')
            it += 2;

        if (it != end && *it != '}')
            throw format_error("invalid format");

        return it;
    }

    template <typename FormatContext>
    auto format(const std::chrono::duration<Rep, Period> &rtime, FormatContext &ctx) -> decltype(ctx.out())
    {
        return format_to(ctx.out(), "{}", mio::to_string(rtime));
    }
};
