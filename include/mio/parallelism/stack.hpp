#pragma once

#include "mio/parallelism/aba_ptr.hpp"
#include "mio/parallelism/allocator.hpp"

namespace mio
{
    namespace parallelism
    {
        template <typename T_>
        class stack
        {
        private:
            struct node
            {
                std::atomic<aba_ptr<node>> next;
                T_ value;
            };

            allocator<node> allocator_;
            std::atomic<aba_ptr<node>> top_;

        public:
            stack()
            {
                top_ = nullptr;
            }
            ~stack()
            {
                while (top_.load())
                {
                    aba_ptr<node> next = top_.load()->next;
                    allocator_.deallocate(top_);
                    top_ = next;
                }
            }

            void push(const T_ &val)
            {
                aba_ptr<node> n = allocator_.allocate();
                n->value = val;

                aba_ptr<node> exp = top_;
                n->next = exp;

                while (!top_.compare_exchange_weak(exp, n))
                {
                    n->next = exp;
                }
            }

            void pop(T_ &val)
            {
                aba_ptr<node> exp = top_;
                aba_ptr<node> ret = exp;

                while (!exp || !top_.compare_exchange_weak(exp, exp->next))
                {
                    while (!exp)
                    {
                        exp = top_;
                    }

                    ret = exp;
                }

                val = std::move(ret->value);

                allocator_.deallocate(exp);
            }
        };

    } // namespace parallelism
} // namespace mio