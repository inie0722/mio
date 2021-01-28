#pragma once

#include <boost/interprocess/offset_ptr.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>

#include <variant>
#include <iostream>

namespace mio
{
    namespace interprocess
    {
        template <typename T>
        using offset_ptr = boost::interprocess::offset_ptr<T, int64_t, uint64_t>;

        using managed_shared_memory = boost::interprocess::basic_managed_shared_memory<char, boost::interprocess::rbtree_best_fit<boost::interprocess::mutex_family, offset_ptr<void>>, boost::interprocess::iset_index>;

        using managed_mapped_file = boost::interprocess::basic_managed_mapped_file<char, boost::interprocess::rbtree_best_fit<boost::interprocess::mutex_family, offset_ptr<void>>, boost::interprocess::iset_index>;

        template <typename T>
        class allocator
        {
        private:
            template <typename>
            friend class allocator;

            using std_allocator_t = std::allocator<T>;
            using segment_manager_t = mio::interprocess::managed_shared_memory::segment_manager;
            using shared_memory_allocator_t = boost::interprocess::allocator<T, segment_manager_t>;

        public:
            using value_type = typename shared_memory_allocator_t::value_type;

            using pointer = typename shared_memory_allocator_t::pointer;

            using const_pointer = typename shared_memory_allocator_t::const_pointer;

            using void_pointer = typename shared_memory_allocator_t::void_pointer;

            using const_void_pointer = const typename shared_memory_allocator_t::void_pointer;

            using difference_type = typename shared_memory_allocator_t::difference_type;

            using size_type = typename shared_memory_allocator_t::size_type;

        private:
            std::variant<std_allocator_t, shared_memory_allocator_t> instance_;

        public:
            allocator()
                : instance_(std_allocator_t())
            {
            }

            allocator(segment_manager_t *segment_mngr)
                : instance_(segment_mngr)
            {
            }

            template <class U>
            allocator(const allocator<U> &other)
            {
                if (std::holds_alternative<typename allocator<U>::std_allocator_t>(other.instance_))
                {
                    new (&instance_) std::variant<std_allocator_t, shared_memory_allocator_t>(std::get<typename allocator<U>::std_allocator_t>(other.instance_));
                }
                else
                {
                    new (&instance_) std::variant<std_allocator_t, shared_memory_allocator_t>(std::get<typename allocator<U>::shared_memory_allocator_t>(other.instance_));
                }
            }

            pointer allocate(size_type n)
            {
                if (std::holds_alternative<std_allocator_t>(instance_))
                {
                    return std::get<std_allocator_t>(instance_).allocate(n);
                }
                else
                {
                    return std::get<shared_memory_allocator_t>(instance_).allocate(n);
                }
            }

            void deallocate(pointer p, size_type n)
            {
                if (std::holds_alternative<std_allocator_t>(instance_))
                {
                    std::get<std_allocator_t>(instance_).deallocate(p.get(), n);
                }
                else
                {
                    std::get<shared_memory_allocator_t>(instance_).deallocate(p, n);
                }
            }
        };
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