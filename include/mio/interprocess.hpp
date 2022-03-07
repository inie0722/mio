/**
 * @file interprocess.hpp
 * @author 然Y (inie0722@gmail.com)
 * @brief
 * @version 0.1
 * @date 2022-03-07
 *
 * @copyright Copyright (c) 2022
 *
 */
#pragma once

#include <boost/interprocess/offset_ptr.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>

#include <cstdint>
#include <variant>
#include <memory>

namespace mio
{
    namespace interprocess
    {
        /// 偏移指针
        template <typename T>
        using offset_ptr = boost::interprocess::offset_ptr<T, int64_t, uint64_t>;

        /// 共享内存
        using managed_shared_memory = boost::interprocess::basic_managed_shared_memory<char, boost::interprocess::rbtree_best_fit<boost::interprocess::mutex_family, offset_ptr<void>>, boost::interprocess::iset_index>;

        /// 文件映射
        using managed_mapped_file = boost::interprocess::basic_managed_mapped_file<char, boost::interprocess::rbtree_best_fit<boost::interprocess::mutex_family, offset_ptr<void>>, boost::interprocess::iset_index>;

        /**
         * @brief 多态分配器
         * @details 可以分配共享内存 也可以直接分配内存
         * @tparam T
         */
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
