#pragma once

#include <type_traits>
#include <tuple>
#include <string>
#include <stdint.h>
#include <chrono>

namespace mio
{
    template <typename T_, typename = void>
    struct is_string : std::false_type
    {
    };

    template <typename T_>
    struct is_string<T_, decltype((void)std::string(std::declval<T_>()))> : std::true_type
    {
    };

    template <typename T_>
    inline bool is_string_v = is_string<T_>::value;

    template <typename... Args>
    class type_tuple
    {
    private:
        template <typename T_>
        struct __tuple
        {
            using type = T_;
        };

        std::tuple<__tuple<Args>...> tuple;

    public:
        template <size_t I>
        struct get
        {
            using type = typename std::remove_reference_t<decltype(std::get<I>(tuple))>::type;
        };
    };

    namespace detail
    {
        template <typename T_>
        constexpr uint8_t get_type_id()
        {
            using T = std::remove_cv_t<T_>;
            if constexpr (std::is_same_v<T, char>)
                return 1;
            else if constexpr (std::is_same_v<T, int8_t>)
                return 2;
            else if constexpr (std::is_same_v<T, uint8_t>)
                return 3;
            else if constexpr (std::is_same_v<T, int16_t>)
                return 4;
            else if constexpr (std::is_same_v<T, uint16_t>)
                return 5;
            else if constexpr (std::is_same_v<T, int32_t>)
                return 6;
            else if constexpr (std::is_same_v<T, uint32_t>)
                return 7;
            else if constexpr (std::is_same_v<T, int64_t>)
                return 8;
            else if constexpr (std::is_same_v<T, uint64_t>)
                return 9;
            else if constexpr (std::is_same_v<T, float>)
                return 10;
            else if constexpr (std::is_same_v<T, double>)
                return 11;
            else if constexpr (std::is_same_v<T, long double>)
                return 12;
            else if constexpr (is_string_v<T>)
                return 13;
            else if constexpr (std::is_same_v<T, std::chrono::nanoseconds>)
                return 14;
        }
    } // namespace detail

    template <typename T>
    struct type_id
    {
        static constexpr uint8_t value = detail::get_type_id<T>();
    };

    template <typename T_>
    inline uint8_t type_id_v = type_id<T_>::value;
} // namespace mio