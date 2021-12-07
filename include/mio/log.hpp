#pragma once

#include <functional>
#include <atomic>
#include <thread>

#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <fmt/format.h>

#include <mio/parallelism/ring_queue.hpp>
#include <mio/chrono.hpp>

namespace mio
{
    class log
    {
    private:
        mio::parallelism::ring_queue<std::function<void()>> queue_;
        boost::interprocess::interprocess_semaphore semaphore_;
        std::atomic<bool> flag_;
        std::thread thread_;

    public:
        log(size_t n)
            : queue_(n), semaphore_(0)
        {
        }

        void run()
        {
            this->flag_ = true;
            thread_ = std::thread([&]()
                                  {
                                      while (flag_ && !queue_.empty())
                                      {
                                          this->semaphore_.wait();

                                          std::function<void()> val;
                                          this->queue_.pop(val);
                                          val();
                                      }
                                  });
        }

        void stop()
        {
            this->flag_ = false;
            thread_.join();
        }

        template <typename Stream, typename... Args>
        void operator()(Stream &stream, const std::string &fmt, const Args &...args)
        {
            queue_.push([=, &stream]()
                        { stream << fmt::format(fmt, args...); });
            semaphore_.post();
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