#pragma once

#include <stddef.h>
#include <stdint.h>
#include <atomic>
#include <tuple>

#include <mio/parallelism/ring_queue.hpp>
#include <mio/chrono.hpp>
#include <mio/parallelism/utility.hpp>

#include <fmt/format.h>

namespace mio
{
    template <size_t QUEUE_SIZE = 1024, size_t BUFFER_SIZE = 65536>
    class log
    {
    private:
        struct message
        {
            void *ptr;
            void (*fun)(void *ptr);
            size_t size;
            std::atomic<uint64_t> *readable_limit;
        };

        class SPSC_buffer
        {
        private:
            char data_[BUFFER_SIZE];
            alignas(CACHE_LINE) std::atomic<uint64_t> writable_limit_ = 0;
            alignas(CACHE_LINE) std::atomic<uint64_t> readable_limit_ = 0;

        public:
            message alloc(size_t size)
            {
                message ret;
                ret.readable_limit = &readable_limit_;
                auto writable_limit = writable_limit_.load();
                auto r = BUFFER_SIZE - writable_limit % BUFFER_SIZE;
                ret.size = r < size ? r + size : size;

                auto writable_index = writable_limit + ret.size % BUFFER_SIZE;

                //写满了等待 可写
                while (BUFFER_SIZE - (writable_limit - readable_limit_) < ret.size)
                    ;

                ret.ptr = &data_[writable_index];
                return ret;
            }

            size_t size() const
            {
                return writable_limit_ - readable_limit_;
            }
        };

        SPSC_buffer buffer_;
        inline static mio::parallelism::ring_queue<message> queue_{QUEUE_SIZE};

    public:
        log() = default;

        ~log()
        {
            while (this->size())
                ;
        }

        template <typename Stream, typename Format, typename... Args>
        void operator()(Stream &stream, const Format &fmt, const Args &...args)
        {
            using args_t = std::tuple<Stream &, Format, Args...>;
            auto msg = buffer_.alloc(sizeof(args_t));
            new (msg.ptr) args_t(stream, fmt, args...);

            msg.fun = [](void *ptr)
            {
                auto *args = reinterpret_cast<args_t *>(ptr);
                std::get<Stream &>(*args) << fmt::format(std::get<Format>(*args), std::get<Args>(*args)...);
            };

            queue_.push(msg);
        }

        size_t size() const
        {
            return buffer_.size();
        }

        static void run_once()
        {
            message msg;
            queue_.pop(msg);
            msg.fun(msg.ptr);
            msg.readable_limit += msg.size;
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