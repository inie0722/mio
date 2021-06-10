#pragma once

#include "mio/parallelism/aba_ptr.hpp"
#include "mio/parallelism/allocator.hpp"

namespace mio
{
    namespace parallelism
    {
        template <typename T>
        class queue
        {
        private:
            struct node
            {
                std::atomic<aba_ptr<node>> next;
                T data;
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

            void push(const T &val)
            {
                aba_ptr<node> n = allocator_.allocate();
                n->data = val;
                n->next = nullptr;

                while (1)
                {
                    aba_ptr<node> tail = tail_.load();
                    aba_ptr<node> next = tail->next.load();
                    aba_ptr<node> tail2 = tail_.load();

                    if (tail == tail2)
                    {
                        if (next == nullptr)
                        {
                            if (tail->next.compare_exchange_weak(next, n))
                            {
                                tail_.compare_exchange_strong(tail, n);
                                return;
                            }
                        }
                        else
                        {
                            tail_.compare_exchange_strong(tail, next);
                        }
                    }
                }
            }

            void pop(T &val)
            {
                while (1)
                {
                    aba_ptr<node> head = head_.load();
                    aba_ptr<node> next = head->next.load();
                    aba_ptr<node> tail = tail_.load();
                    aba_ptr<node> head2 = head_.load();

                    if (head == head2)
                    {
                        if (head.get() == tail.get())
                        {
                            if (next == nullptr)
                                continue;
                            tail_.compare_exchange_strong(tail, next);
                        }
                        else
                        {
                            if (next == nullptr)
                                continue;

                            val = next->data;
                            if (head_.compare_exchange_weak(head, next))
                            {
                                allocator_.deallocate(head);
                                return;
                            }
                        }
                    }
                }
            }
        };
    } // namespace parallelism
} // namespace mio