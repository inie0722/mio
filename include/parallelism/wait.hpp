#pragma once

#include <stdint.h>

#include <thread>
#include <functional>

namespace mio
{
    namespace parallelism
    {
        namespace wait
        {
            typedef std::function<void(size_t)> handler_t;

            inline auto active = [](size_t) {};
            inline auto yield = [](size_t) { std::this_thread::yield(); };
        } // namespace wait
    }     // namespace parallelism
} // namespace mio