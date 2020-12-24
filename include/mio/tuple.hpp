#pragma once

#include <mio/type_traits.hpp>

#include <tuple>
#include <boost/pfr.hpp>

namespace mio
{
    template <std::size_t I_, typename T_>
    auto &get(T_ &tuple)
    {
        if constexpr (is_tuple_v<T_>)
        {
            return std::get<I_>(tuple);
        }
        else
        {
            return boost::pfr::get<I_>(tuple);
        }
    }

    namespace detail
    {
        template <typename T_>
        constexpr size_t tuple_size()
        {
            if constexpr (is_tuple_v<T_>)
            {
                return std::tuple_size_v<T_>;
            }
            else
            {
                return boost::pfr::tuple_size_v<T_>;
            }
        }
    } // namespace detail

    template <typename T_>
    struct tuple_size
    {
        static constexpr size_t value = detail::tuple_size<T_>();
    };

    template <typename T_>
    inline constexpr size_t tuple_size_v = tuple_size<T_>::value;
} // namespace mio