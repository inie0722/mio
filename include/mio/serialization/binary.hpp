#pragma once

#pragma once

#include <stdint.h>
#include <assert.h>
#include <array>
#include <string>
#include <string.h>
#include <string_view>
#include <type_traits>
#include <functional>
#include <algorithm>

#include "tool/metaprogramming.hpp"
#include "error/assertion.hpp"

namespace serialization
{
    template <bool TYPE_SAFE>
    class Binary
    {
    private:
        char *data;

        uint64_t read_size;
        uint64_t write_size;

    public:
        Binary(void *buf) : data((char *)buf), read_size(0), write_size(0)
        {
        }

        template <typename T>
        Binary &operator>>(T &input)
        {
            using namespace tool;
            //如果需要保证类型安全
            if constexpr (TYPE_SAFE)
            {
                static_assert(type_id<T>::value, "Type not supported");
                assertion(data[read_size] == type_id<T>::value, "Type not supported");
                read_size++;
            }

            if constexpr (is_string<T>::value)
            {
                std::string_view str = std::string_view(&data[read_size]);

                if constexpr (std::is_pointer<T>::value)
                {
                    memcpy(input, str.data(), str.length() + 1);
                }
                else
                {
                    input = str;
                }

                read_size += str.length() + 1;
            }
            else
            {
                T *d = (T *)&data[read_size];
                read_size += sizeof(T);
                input = *d;
            }

            return *this;
        }

        template <typename T>
        Binary &operator<<(const T &output)
        {
            using namespace tool;
            //如果需要保证类型安全
            if constexpr (TYPE_SAFE)
            {
                static_assert(type_id<T>::value, "Type not supported");
                data[write_size] = type_id<T>::value;
                write_size++;
            }

            if constexpr (is_string<T>::value)
            {
                std::string_view str(output);
                memcpy(&data[write_size], str.data(), str.length() + 1);
                write_size += str.length() + 1;
            }
            else
            {
                memcpy(&data[write_size], &output, sizeof(output));
                write_size += sizeof(T);
            }

            return *this;
        }

        template <typename... Args>
        void pack(const Args &... args)
        {
            (*this << ... << args); 
        }

        template <size_t SIZE, typename... Args>
        static constexpr auto compiletime_pack(const Args &... args)
        {
            using namespace tool;
            std::array<char, SIZE + (TYPE_SAFE ? sizeof...(Args) : 0)> buf{};
            char *p = buf.data();

            auto cpy = [&](auto arg) {
                if constexpr (TYPE_SAFE)
                {
                    static_assert(type_id<decltype(arg)>::value, "Type not supported");
                    *p = type_id<decltype(arg)>::value;
                    p++;
                }

                if constexpr (is_string<decltype(arg)>::value)
                {
                    for (size_t i = 0; i < get_size(arg); i++)
                    {
                        p[i] = arg[i];
                    }
                }
                else if constexpr (std::is_integral<decltype(arg)>::value)
                {
                    for (size_t i = 0; i < get_size(arg); i++)
                    {
                        p[i] = arg & 0xff;
                        arg >>= 8;
                    }
                }

                p += get_size(arg);
            };

            (cpy(args), ...);

            return buf;
        }

        uint8_t get_type_id()
        {
            return data[read_size];
        }

        uint64_t get_read_size()
        {
            return read_size;
        }

        uint64_t get_write_size()
        {
            return write_size;
        }
    };
} // namespace serialization
