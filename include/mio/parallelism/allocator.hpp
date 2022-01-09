#pragma once

#include <cstddef>
#include <atomic>
#include <type_traits>

#include <mio/parallelism/aba_ptr.hpp>

namespace mio
{
    namespace parallelism
    {
        template <typename T>
        class allocator
        {
        public:
            using value_type = T;
            using pointer = aba_ptr<T>;

        private:
            union node
            {
                std::atomic<aba_ptr<node>> next = aba_ptr<node>(nullptr);
                value_type data;
            };

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

                    delete free_list_.load().get();

                    free_list_ = next;
                }
            }

            template <class T1, class T2>
            friend bool operator==(const mio::parallelism::allocator<T1> &lhs, const mio::parallelism::allocator<T2> &rhs) noexcept
            {
                return lhs.free_list_.load() == rhs.free_list_.load();
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
                        return pointer(reinterpret_cast<value_type *>(new node));
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
        };
    } // namespace parallelism
} // namespace mio