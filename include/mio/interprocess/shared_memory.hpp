#pragma once

#include <boost/interprocess/offset_ptr.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>

namespace mio
{
    namespace interprocess
    {
        template <typename T>
        using offset_ptr = boost::interprocess::offset_ptr<T, int64_t, uint64_t>;

        using managed_shared_memory = boost::interprocess::basic_managed_shared_memory<char, boost::interprocess::rbtree_best_fit<boost::interprocess::mutex_family, offset_ptr<void>>, boost::interprocess::iset_index>;

        using managed_mapped_file = boost::interprocess::basic_managed_mapped_file<char, boost::interprocess::rbtree_best_fit<boost::interprocess::mutex_family, offset_ptr<void>>, boost::interprocess::iset_index>;

        template <typename T>
        using allocator = boost::interprocess::allocator<T, mio::interprocess::managed_shared_memory::segment_manager>;
    } // namespace interprocess
} // namespace mio

namespace std
{
    template <typename T>
    class allocator_traits<mio::interprocess::allocator<T>>
    {
    public:
        using allocator_type = mio::interprocess::allocator<T>;

        using value_type = typename allocator_type::value_type;

        using pointer = typename allocator_type::pointer;

        using const_pointer = typename allocator_type::const_pointer;

        using void_pointer = typename allocator_type::void_pointer;

        using const_void_pointer = const typename allocator_type::void_pointer;

        using difference_type = typename allocator_type::difference_type;

        using size_type = typename allocator_type::size_type;

        template <typename T_>
        using rebind_traits = allocator_traits<mio::interprocess::allocator<T_>>;

        inline static pointer allocate(allocator_type &alloc, size_type n)
        {
            return alloc.allocate(n);
        }

        inline static void deallocate(allocator_type &alloc, pointer p, size_type n)
        {
            alloc.deallocate(p, n);
        }
    };
} // namespace std