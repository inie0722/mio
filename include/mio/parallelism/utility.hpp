#pragma once

#include <cstddef>

namespace mio
{
    namespace parallelism
    {
#ifdef __x86_64__
        inline constexpr std::size_t hardware_destructive_interference_size = 64;
        inline constexpr std::size_t hardware_constructive_interference_size = 64;
#else
        static_assert(0, "Does not support current cpu architecture");
#endif
    } // namespace parallelism
} // namespace mio