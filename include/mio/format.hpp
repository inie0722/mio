#pragma once

#include <chrono>

#include <fmt/format.h>

#include <mio/chrono.hpp>

template <typename Rep, typename Period>
struct fmt::formatter<std::chrono::duration<Rep, Period>>
{
    constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin())
    {
        auto it = ctx.begin();
        auto end = ctx.end();

        if (it != end && it[0] == 't' && it[1] == 'm')
            it += 2;

        if (it != end && *it != '}')
            throw format_error("invalid format");

        return it;
    }

    template <typename FormatContext>
    auto format(const std::chrono::duration<Rep, Period> &rtime, FormatContext &ctx) -> decltype(ctx.out())
    {
        return format_to(ctx.out(), "{}", mio::to_string(rtime));
    }
};
