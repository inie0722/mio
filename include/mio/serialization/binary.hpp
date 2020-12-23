#pragma once

#pragma once

#include <stdint.h>
#include <string.h>
#include <string>
#include <string_view>
#include <type_traits>

#include "mio/type_traits.hpp"

namespace mio
{
    namespace serialization
    {
        class binary
        {
        private:
            char *data;
            uint64_t read_size = 0;
            uint64_t write_size = 0;

            template <typename T_>
            constexpr size_t get_arg_size(const T_ &arg)
            {
                if constexpr (is_string_v<T_>)
                    return std::string_view(arg).length() + 1;
                else
                    return sizeof(T_);
            }

            template <typename... Args_>
            constexpr size_t get_args_size(const Args_ &... args)
            {
                return (get_arg_size(args) + ...);
            }

        public:
            binary(void *buf) : data((char *)buf)
            {
            }

            template <typename T_>
            binary &operator>>(T_ &input)
            {
                static_assert(type_id_v<T_>, "Type not supported");

                if (data[read_size] != type_id_v<T_>)
                    throw std::runtime_error("Type not supported");

                read_size++;

                if constexpr (is_string_v<T_>)
                {
                    std::string_view str = std::string_view(&data[read_size]);

                    if constexpr (std::is_pointer_v<T_>)
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
                    T_ *d = (T_ *)&data[read_size];
                    read_size += sizeof(T_);
                    input = *d;
                }

                return *this;
            }

            template <typename T_>
            binary &operator<<(const T_ &output)
            {
                static_assert(type_id_v<T_>, "Type not supported");
                data[write_size] = type_id_v<T_>;
                write_size++;

                if constexpr (is_string_v<T_>)
                {
                    std::string_view str(output);
                    memcpy(&data[write_size], str.data(), str.length() + 1);
                    write_size += str.length() + 1;
                }
                else
                {
                    memcpy(&data[write_size], &output, sizeof(output));
                    write_size += sizeof(T_);
                }

                return *this;
            }

            template <typename... Args_>
            binary& pack(const Args_ &... args)
            {
                return (*this << ... << args);
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
} // namespace mio