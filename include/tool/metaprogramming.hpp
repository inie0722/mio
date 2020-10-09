#pragma once

#include <type_traits>
#include <string>
#include <string_view>
#include <stdint.h>
#include <chrono>

namespace tool
{
    template<typename T, typename U>
    struct is_same_ignore_modifier
    {
        static constexpr bool value
        = std::is_same<T, U>::value 
        || std::is_same<T, const U>::value
        || std::is_same<T, volatile U>::value
        || std::is_same<T, const volatile U>::value
        || std::is_same<const T, U>::value
        || std::is_same<volatile T, U>::value
        || std::is_same<const volatile T, U>::value;
    };

    template<typename T>
    struct is_string
    {
        static constexpr bool value 
        = is_same_ignore_modifier<T, std::string>::value 
        || is_same_ignore_modifier<T, std::string_view>::value 
        || is_same_ignore_modifier<T, char*>::value
        || is_same_ignore_modifier<T, const char*>::value
        || (std::is_array<T>::value ? is_same_ignore_modifier<T, char[sizeof(T)]>::value : 0);
    };

    template <typename T>
    constexpr size_t strlen(const T &str)
    {
        return std::string_view(str).length();
    }

    template <typename T>
    constexpr size_t get_size(const T &d)
    {
        if constexpr (is_string<T>::value)
            return strlen(d) + 1;
        else
            return sizeof(T);
    }

    template <typename... Args>
    constexpr size_t get_size(const Args &... args)
    {
        return (get_size(args) + ...);
    }

    template <typename... Args>
    constexpr size_t get_args_size(const Args &... args)
    {
        return sizeof...(Args);
    }

    namespace detail
    {
        template <typename T>
        constexpr uint8_t get_type_id()
        {
            if constexpr (is_same_ignore_modifier<T, char>::value)
                return 1;
            else if constexpr (is_same_ignore_modifier<T, int8_t>::value)
                return 2;
            else if constexpr (is_same_ignore_modifier<T, uint8_t>::value)
                return 3;
            else if constexpr (is_same_ignore_modifier<T, int16_t>::value)
                return 4;
            else if constexpr (is_same_ignore_modifier<T, uint16_t>::value)
                return 5;
            else if constexpr (is_same_ignore_modifier<T, int32_t>::value)
                return 6;
            else if constexpr (is_same_ignore_modifier<T, uint32_t>::value)
                return 7;
            else if constexpr (is_same_ignore_modifier<T, int64_t>::value)
                return 8;
            else if constexpr (is_same_ignore_modifier<T, uint64_t>::value)
                return 9;
            else if constexpr (is_same_ignore_modifier<T, float>::value)
                return 10;
            else if constexpr (is_same_ignore_modifier<T, double>::value)
                return 11;
            else if constexpr (is_same_ignore_modifier<T, long double>::value)
                return 12;
            else if constexpr (is_string<T>::value)
                return 13;
            else if constexpr (is_same_ignore_modifier<T, std::chrono::nanoseconds>::value)
                return 14;
        }
    } // namespace detail

    template<typename T>
    struct type_id
    {
        static constexpr uint8_t value = detail::get_type_id<T>();
    };
    
} // namespace tool
