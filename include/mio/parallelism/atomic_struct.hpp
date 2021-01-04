#pragma once

#include <mutex>
#include <shared_mutex>
#include <atomic>

namespace mio
{
    namespace parallelism
    {
        template <typename T_>
        class atomic_struct
        {
        private:
            std::atomic<bool> flag = false;
            std::shared_mutex mutex[2];
            T_ data[2];

        public:
            atomic_struct() = default;

            atomic_struct(const T_ &other)
            {
                data[0] = other;
                data[1] = other;
            }

            atomic_struct &operator=(const T_ &other)
            {
                bool index = flag;
                std::lock_guard lock(mutex[index]);
                data[index] = other;
                flag = !index;

                return *this;
            }

            operator T_() const
            {
                bool index = !flag.load();
                std::shared_lock lock(const_cast<std::shared_mutex&>(mutex[index]));
                return data[index];
            }
        };
    } // namespace parallelism
} // namespace mio