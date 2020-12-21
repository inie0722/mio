#pragma once

#include <type_traits>
#include <tuple>
#include <string>
#include <string_view>

namespace mio
{
    template <typename T_>
    struct is_string
    {
    private:
        using type = typename std::remove_cv<T_>::type;

    public:
        static constexpr bool value = std::is_same<type, std::string>::value || std::is_same<type, std::string_view>::value || std::is_same<type, char *>::value || std::is_same<type, const char *>::value || (std::is_array<type>::value ? std::is_same<type, char[sizeof(type)]>::value : 0);
    };

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
            using type = typename std::remove_reference<decltype(std::get<I>(tuple))>::type::type;
        };
    };
} // namespace mio