#pragma once

#include "synchronization/spin_shared_mutex.hpp"

#include <memory>

namespace libcpp
{
    template <typename T>
    class atomic_shared_ptr
    {
    private:
        std::shared_ptr<T> ptr;
        spin_shared_mutex mutex;

    public:
        atomic_shared_ptr()
        {}
        
        atomic_shared_ptr(std::shared_ptr<T> val) : ptr(val)
        {
        }

        atomic_shared_ptr &operator=(std::shared_ptr<T> &val)
        {
            store(val);
            return *this;
        }

        bool is_lock_free() const
        {
            return false;
        }

        void store(std::shared_ptr<T> &val)
        {
            mutex.lock();
            ptr = val;
            mutex.unlock();
        }

        std::shared_ptr<T> load()
        {
            mutex.lock_shared();
            auto ret = ptr;
            mutex.unlock_shared();

            return ret;
        }

        operator std::shared_ptr<T>()
        {
            return load();
        }

        std::shared_ptr<T> exchange(std::shared_ptr<T> &val)
        {
            mutex.lock();
            auto ret = ptr;
            ptr = val;
            mutex.unlock();
            return ret;
        }

        bool compare_exchange_weak(std::shared_ptr<T> &expected, std::shared_ptr<T> desired)
        {
            bool ret;
            mutex.lock();
            if (ptr == expected)
            {
                ptr = desired;
                ret = true;
            }
            else
            {
                expected = ptr;
                ret = false;
            }

            mutex.unlock();
            return ret;
        }

        bool compare_exchange_strong(std::shared_ptr<T> &expected, std::shared_ptr<T> desired)
        {
            return compare_exchange_weak(expected, desired);
        }
    };
} // namespace libcpp