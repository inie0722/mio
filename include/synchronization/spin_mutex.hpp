#pragma once

#include <atomic>
#include <thread>

namespace libcpp
{
    class spin_mutex
    {
    private:
        std::atomic_flag flag = false;

    public:
        void lock()
        {
            size_t count = 0;
            while (flag.test_and_set(std::memory_order_acquire))
            {
                if(count > 8)
                {
                    std::this_thread::yield();
                }
                ++count;
            }
        }

        bool try_lock()
        {
            return !flag.test_and_set(std::memory_order_release);
        }

        void unlock()
        {
            flag.clear();
        }
    };
} // namespace libcpp