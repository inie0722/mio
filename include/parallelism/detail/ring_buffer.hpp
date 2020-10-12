#pragma once

#include <stddef.h>

namespace mio
{
    namespace parallelism
    {
        namespace detail
        {
            template <typename T_, size_t SIZE_>
            class ring_buffer
            {
            private:
                struct
                {
                    alignas(64) T_ data;
                } array[SIZE_];

                size_t get_index(const size_t index) const
                {
                    return index % SIZE_;
                }

            public:
                T_ &operator[](const size_t index)
                {
                    return this->array[get_index(index)].data;
                }
            };
        } // namespace detail
    }     // namespace parallelism
} // namespace mio
