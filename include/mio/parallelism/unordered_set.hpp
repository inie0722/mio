#pragma once

#include <parallel_hashmap/phmap.h>

namespace mio
{
    namespace parallelism
    {
        template <class T,
                  class Hash = phmap::priv::hash_default_hash<T>,
                  class Eq = phmap::priv::hash_default_eq<T>,
                  class Alloc = phmap::priv::Allocator<T>>
        using unordered_set = phmap::parallel_node_hash_set<T, Hash, Eq, Alloc>;
    }
} // namespace mio