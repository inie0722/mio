#pragma once

#include <stdint.h>

namespace libcpp
{
    namespace network
    {
        bool is_little()
        {
            uint16_t a = 1;

            return (*(char *)&a);
        }

        template <typename T>
        T hton(T val)
        {
            T ret;
            if (is_little())
            {
                for (size_t i = 0; i < sizeof(T); i++)
                {
                    ((char *)&ret)[i] = ((char *)&val)[sizeof(T) - i - 1];
                }
            }
            else
            {
                ret = val;
            }
            return ret;
        }

        template <typename T>
        T ntoh(T val)
        {
            T ret;
            if (is_little())
            {
                for (size_t i = 0; i < sizeof(T); i++)
                {
                    ((char *)&ret)[i] = ((char *)&val)[sizeof(T) - i - 1];
                }
            }
            else
            {
                ret = val;
            }
            return ret;
        }
    } // namespace network
} // namespace libcpp