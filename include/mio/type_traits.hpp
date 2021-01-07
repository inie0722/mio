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
    inline constexpr bool is_string_v = is_string<T_>::value;

    template <typename T_, typename = void>
    struct is_container : std::false_type
    {
    };

    template <typename T_>
    struct is_container<T_, decltype((void)(std::declval<T_>().begin() != std::declval<T_>().end()))> : std::true_type
    {
    };

    template <typename T_>
    inline constexpr bool is_container_v = is_container<T_>::value;

    template <typename T_, typename = void>
    struct is_tuple : std::false_type
    {
    };

    template <typename T_>
    struct is_tuple<T_, decltype((void)std::get<0>(std::declval<T_>()))> : std::true_type
    {
    };

    template <typename T_>
    inline constexpr bool is_tuple_v = is_tuple<T_>::value;

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
            if constexpr (std::is_same_v<T, void>)
                return 0;
            else if constexpr (std::is_same_v<T, char>)
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
            else if constexpr (std::is_same_v<T, std::chrono::nanoseconds>)
                return 13;
            else if constexpr (is_string_v<T>)
                return 14;
        }
    } // namespace detail

    template <typename T>
    struct type_id
    {
        static constexpr uint8_t value = detail::get_type_id<T>();
    };

    template <typename T_>
    inline constexpr uint8_t type_id_v = type_id<T_>::value;

    template <typename T_>
    struct remove_pointer
    {
        using type = std::remove_reference_t<decltype(*std::declval<T_>())>;
    };

    template <typename T_>
    using remove_pointer_t = typename remove_pointer<T_>::type;

    namespace detail
    {
        template <typename T_, bool>
        struct remove_container_type
        {
        };

        template <typename T_>
        struct remove_container_type<T_, true>
        {
            using type = typename T_::value_type;
        };

        template <typename T_>
        struct remove_container_type<T_, false>
        {
            using type = remove_pointer_t<T_>;
        };
    } // namespace detail

    template <typename T_>
    struct remove_container_type
    {
        using type = typename detail::remove_container_type<T_, is_container_v<T_>>::type;
    };

    template <typename T_>
    using remove_container_t = typename remove_container_type<T_>::type;
} // namespace mio