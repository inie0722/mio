#pragma once

#include <type_traits>

namespace mio
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
}