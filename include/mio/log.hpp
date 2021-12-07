#pragma once

#include <functional>
#include <atomic>
#include <thread>

#include <boost/interprocess/sync/interprocess_semaphore.hpp>
#include <fmt/format.h>

#include <mio/parallelism/ring_queue.hpp>

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