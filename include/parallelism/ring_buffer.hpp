#pragma once

#include <stddef.h>
#include <string.h>

namespace libcpp
{
    namespace detail
    {
        template <typename T, size_t SIZE>
        class ring_buffer
        {
        private:
            struct
            {
                alignas(64) T data;
            } array[SIZE];

            static constexpr size_t pow_2(const size_t num)
            {
                size_t t_num = num;

                if ((t_num & (t_num - 1))) //去掉一个1，判断是否为0
                    return 0;

                size_t i = 0;
                for (; t_num; i++)
                {
                    t_num = t_num >> 1;
                }

                return i;
            }

            size_t get_index(const size_t index) const
            {
                return index & (SIZE - 1);
            }

        public:
            ring_buffer()
            {
                memset(array, 0, sizeof(array));
                static_assert(pow_2(SIZE), "size is not a power of two\n");
            }

            T &operator[](const size_t index)
            {
                return this->array[get_index(index)].data;
            }
        };
    } // namespace detail
} // namespace libcpp
