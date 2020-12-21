#pragma once

#include <stddef.h>
#include <type_traits>

#include "mio/parallelism/aba_ptr.hpp"

namespace mio
{
    namespace parallelism
    {
        template <typename T_>
        class allocator
        {
        public:
            using value_type = T_;
            using size_type = size_t;
            using difference_type = ptrdiff_t;
            using propagate_on_container_move_assignment = std::true_type;

        private:
            union alignas(alignof(std::atomic<aba_ptr<void>>) > alignof(T_) ? alignof(std::atomic<aba_ptr<void>>) : alignof(T_)) node
            {
                std::atomic<aba_ptr<node>> next;
                char data[sizeof(T_)];
            };

            std::atomic<aba_ptr<node>> free_list;

        public:
            allocator()
            {
                free_list = nullptr;
            }

            ~allocator()
            {
                while (free_list.load())
                {
                    aba_ptr<node> next = free_list.load()->next;

                    delete free_list.load();

                    free_list = next;
                }
            }

            aba_ptr<T_> allocate()
            {
                aba_ptr<node> exp = free_list;
                aba_ptr<node> ret = exp;

                while (!exp || !free_list.compare_exchange_weak(exp, exp->next))
                {
                    if (exp == nullptr)
                    {
                        ret = new node;
                        return static_cast<aba_ptr<T_>>(ret);
                    }

                    ret = exp;
                }

                return static_cast<aba_ptr<T_>>(ret);
            }

            void deallocate(aba_ptr<T_> p)
            {
                aba_ptr<node> n = static_cast<aba_ptr<node>>(p);
                aba_ptr<node> exp = free_list;

                n->next = exp;
                while (!free_list.compare_exchange_weak(exp, n))
                {
                    n->next = exp;
                }
            }
        };
    } // namespace parallelism
} // namespace mio

