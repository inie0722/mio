#pragma once

#include "mio/parallelism/aba_ptr.hpp"
#include "mio/parallelism/allocator.hpp"

namespace mio
{
    namespace parallelism
    {
        template <typename T_>
        class queue
        {
        private:
            struct node
            {
                std::atomic<aba_ptr<node>> next;
                T_ value;
            };

            allocator<node> allocator_;
            std::atomic<aba_ptr<node>> head_;
            std::atomic<aba_ptr<node>> tail_;

        public:
            queue()
            {
                //先构造一个头节点
                head_ = allocator_.allocate();
                head_.load()->next = nullptr;
                tail_ = head_.load();
            }

            ~queue()
            {
                while (head_.load())
                {
                    aba_ptr<node> next = head_.load()->next;
                    allocator_.deallocate(head_);
                    head_ = next;
                }
            }

            void push(const T_ &val)
            {
                auto n = allocator_.allocate();
                n->value = val;
                n->next = nullptr;

                while (1)
                {
                    aba_ptr<node> last = tail_.load();
                    aba_ptr<node> exp = nullptr;

                    //如果 last next 是 null 就代表是最后一个节点
                    if (last->next.compare_exchange_weak(exp, n))
                    {
                        //将tail 指向新节点
                        tail_.compare_exchange_weak(last, n);
                        return;
                    }
                }
            }

            void push(T_ &&val)
            {
                auto n = allocator_.allocate();
                n->value = std::move(val);
                n->next = nullptr;

                while (1)
                {
                    aba_ptr<node> last = tail_.load();
                    aba_ptr<node> exp = nullptr;

                    //如果 last next 是 null 就代表是最后一个节点
                    if (last->next.compare_exchange_weak(exp, n))
                    {
                        //将tail 指向新节点
                        tail_.compare_exchange_weak(last, n);
                        return;
                    }
                }
            }

            void pop(T_ &val)
            {
                while (1)
                {
                    aba_ptr<node> first = head_;
                    aba_ptr<node> last = tail_;
                    aba_ptr<node> next = first->next;

                    //如果队列中有元素
                    if (first != last)
                    {
                        if (head_.compare_exchange_weak(first, next))
                        {
                            //删除 first
                            allocator_.deallocate(first);
                            //next 节点变为头节点
                            val = std::move(next->value);
                            return;
                        }
                    }
                }
            }
        };
    } // namespace parallelism
} // namespace mio