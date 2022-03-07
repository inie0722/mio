/**
 * @file unordered_set.hpp
 * @author 然Y (inie0722@gmail.com)
 * @brief
 * @version 0.1
 * @date 2022-03-07
 *
 * @copyright Copyright (c) 2022
 *
 */
#pragma once

#include <parallel_hashmap/phmap.h>

namespace mio
{
    namespace parallelism
    {
        /**
         * @brief unordered_set 才用GitHub parallel_hashmap
         *
         * @tparam T
         * @tparam Hash
         * @tparam Eq
         * @tparam Alloc
         */
        template <class T,
                  class Hash = phmap::priv::hash_default_hash<T>,
                  class Eq = phmap::priv::hash_default_eq<T>,
                  class Alloc = phmap::priv::Allocator<T>>
        using unordered_set = phmap::parallel_node_hash_set<T, Hash, Eq, Alloc>;
    }
} // namespace mio