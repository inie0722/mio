#pragma once

#include <cstddef>
#include <atomic>
#include <type_traits>
#include <memory>

#include <mio/parallelism/aba_ptr.hpp>

namespace mio
{
    namespace parallelism
    {
        template <typename T, typename Allocator = std::allocator<T>>
        class allocator
        {
        public:
            using allocator_type = Allocator;
            using value_type = T;
            using pointer = aba_ptr<T>;

        private:
            union node
            {
                std::atomic<aba_ptr<node>> next;
                value_type data;
            };

            typename std::allocator_traits<Allocator>::rebind_alloc<node> alloc_;

            std::atomic<aba_ptr<node>> free_list_ = aba_ptr<node>(nullptr);

        public:
            allocator() = default;

            allocator(const allocator &) = delete;
            allocator &operator=(const allocator &) = delete;

            ~allocator()
            {
                while (free_list_.load())
                {
                    aba_ptr<node> next = free_list_.load()->next;

                    alloc_.deallocate(free_list_.load().get(), 1);

                    free_list_ = next;
                }
            }

            pointer allocate(std::size_t n)
            {
                if (n != 1)
                {
                    throw std::invalid_argument("n should be equal to 1");
                }

                aba_ptr<node> exp = free_list_.load();

                while (1)
                {
                    if (!exp)
                    {
                        return pointer(reinterpret_cast<value_type *>(alloc_.allocate(1)));
                    }

                    aba_ptr<node> next = exp->next;
                    if (free_list_.compare_exchange_weak(exp, next))
                    {
                        return reinterpret_cast<pointer &>(exp);
                    }
                }
            }

            void deallocate(pointer p, std::size_t n)
            {
                if (n != 1)
                {
                    throw std::invalid_argument("n should be equal to 1");
                }

                aba_ptr<node> node_ptr = reinterpret_cast<aba_ptr<node> &>(p);

                while (1)
                {
                    aba_ptr<node> exp = free_list_;
                    node_ptr->next = free_list_.load();

                    if (free_list_.compare_exchange_weak(exp, node_ptr))
                        return;
                }
            }

            bool is_lock_free() const noexcept
            {
                return free_list_.is_lock_free();
            }

            template <class T1, class T2>
            friend bool operator==(const mio::parallelism::allocator<T1> &lhs, const mio::parallelism::allocator<T2> &rhs) noexcept;
        };
    } // namespace parallelism
} // namespace mio

template <class T1, class T2>
bool operator==(const mio::parallelism::allocator<T1> &lhs, const mio::parallelism::allocator<T2> &rhs) noexcept
{
    return lhs.free_list_.load() == rhs.free_list_.load();
}