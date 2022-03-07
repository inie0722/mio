/**
 * @file unordered_map.hpp
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
         * @brief unordered_map 才用GitHub parallel_hashmap
         *
         * @tparam Key
         * @tparam Value
         * @tparam Hash
         * @tparam Eq
         * @tparam Alloc
         */
        template <class Key, class Value,
                  class Hash = phmap::priv::hash_default_hash<Key>,
                  class Eq = phmap::priv::hash_default_eq<Key>,
                  class Alloc = phmap::priv::Allocator<phmap::priv::Pair<const Key, Value>>>
        using unordered_map = phmap::parallel_node_hash_map<Key, Value, Hash, Eq, Alloc>;
    }
} // namespace mio