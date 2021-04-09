#pragma once

#include <array>

#include <mio/metaprogram/hash.hpp>

#define MIO_RAND() (static_cast<int64_t>(mio::metaprogram::hash{__COUNTER__ + mio::metaprogram::hash{}(__TIME__)}(__PRETTY_FUNCTION__)))

namespace mio
{
    namespace metaprogram
    {
        template <size_t N>
        constexpr auto rand(uint64_t seed)
        {
            std::array<int64_t, N> ret = {};

            for (size_t i = 0; i < N; i++)
            {
                ret[i] = static_cast<int64_t>(mio::metaprogram::hash{seed}(i));
            }

            return ret;
        }
    }
}