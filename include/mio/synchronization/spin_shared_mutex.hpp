#pragma once

#include <tbb/spin_rw_mutex.h>

namespace libcpp
{
    class spin_shared_mutex
    {
    private:
        tbb::spin_rw_mutex mutex;

    public:
        void lock()
        {
            mutex.lock();
        }

        bool try_lock()
        {
            return mutex.try_lock();
        }

        void unlock()
        {
            mutex.unlock();
        }

        void lock_shared()
        {
            mutex.lock_read();
        }

        bool try_lock_shared()
        {
            return mutex.try_lock_read();
        }

        void unlock_shared()
        {
            mutex.unlock();
        }
    };
} // namespace libcpp