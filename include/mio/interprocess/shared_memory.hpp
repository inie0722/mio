#pragma once

#include <boost/interprocess/offset_ptr.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>

namespace mio
{
    namespace parallelism
    {
        using offset_ptr = boost::interprocess::offset_ptr<void, int64_t, uint64_t>;

        using managed_shared_memory = boost::interprocess::basic_managed_shared_memory<char, boost::interprocess::rbtree_best_fit<boost::interprocess::mutex_family, offset_ptr>, boost::interprocess::iset_index>;

        using managed_mapped_file = boost::interprocess::basic_managed_mapped_file<char, boost::interprocess::rbtree_best_fit<boost::interprocess::mutex_family, offset_ptr>, boost::interprocess::iset_index>;
    }
}
