/**
 * @file utility.hpp
 * @author 然Y (inie0722@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2022-03-07
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#pragma once

#include <cstddef>
#include <new>

namespace mio
{
    namespace parallelism
    {
#if defined __cpp_lib_hardware_interference_size
        inline constexpr std::size_t hardware_destructive_interference_size = std::hardware_destructive_interference_size;
        inline constexpr std::size_t hardware_constructive_interference_size = std::hardware_constructive_interference_size;
#elif defined __x86_64__
        inline constexpr std::size_t hardware_destructive_interference_size = 64;
        inline constexpr std::size_t hardware_constructive_interference_size = 64;
#else
        static_assert(0, "Does not support current cpu architecture");
#endif
    } // namespace parallelism
} // namespace mio