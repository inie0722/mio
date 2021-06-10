#pragma once

#include <stddef.h>
#include <type_traits>

#include "mio/parallelism/aba_ptr.hpp"

namespace mio
{
    namespace parallelism
    {
        template <typename T>
        class allocator
        {
        public:
            using value_type = T;
            using size_type = size_t;
            using difference_type = ptrdiff_t;
            using propagate_on_container_move_assignment = std::true_type;

        private:
            union alignas(alignof(std::atomic<aba_ptr<void>>) > alignof(T) ? alignof(std::atomic<aba_ptr<void>>) : alignof(T)) node
            {
                std::atomic<aba_ptr<node>> next;
                char data[sizeof(T)];
            };

            std::atomic<aba_ptr<node>> free_list_;

        public:
            allocator()
            {
                free_list_ = nullptr;
            }

            ~allocator()
            {
                while (free_list_.load())
                {
                    aba_ptr<node> next = free_list_.load()->next;

                    delete free_list_.load().get();

                    free_list_ = next;
                }
            }

            aba_ptr<T> allocate()
            {
                aba_ptr<node> exp = free_list_.load();

                while (1)
                {
                    if (!exp)
                    {
                        return new T;
                    }

                    aba_ptr<node> next = exp->next;
                    if (free_list_.compare_exchange_weak(exp, next))
                    {
                        return static_cast<aba_ptr<T>>(exp);
                    }
                }
            }

            void deallocate(aba_ptr<T> p)
            {
                aba_ptr<node> n = static_cast<aba_ptr<node>>(p);

                while (1)
                {
                    aba_ptr<node> exp = free_list_;
                    n->next = free_list_.load();

                    if (free_list_.compare_exchange_weak(exp, n))
                        return;
                }
            }
        };
    } // namespace parallelism
} // namespace mio
