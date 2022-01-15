#pragma once

#include "mio/parallelism/aba_ptr.hpp"
#include "mio/parallelism/allocator.hpp"

namespace mio
{
    namespace parallelism
    {
        template <typename T, typename Allocator = std::allocator<T>>
        class queue
        {
        public:
            using value_type = T;
            using size_type = std::size_t;

        private:
            struct node
            {
                std::atomic<aba_ptr<node>> next;
                T data;
            };

            using allocator_type = typename mio::parallelism::allocator<node, typename std::allocator_traits<Allocator>::rebind_alloc<node>>;
            using allocator_traits = std::allocator_traits<allocator_type>;

            allocator_type alloc_;
            std::atomic<aba_ptr<node>> head_;
            std::atomic<aba_ptr<node>> tail_;
            std::atomic<size_t> size_ = 0;

        public:
            queue()
            {
                //先构造一个头节点
                aba_ptr<node> n = this->alloc_.allocate(1);
                allocator_traits::construct(this->alloc_, n.get(), aba_ptr<node>(nullptr));
                this->head_ = n;
                this->tail_ = n;
            }

            ~queue()
            {
                while (this->head_.load())
                {
                    aba_ptr<node> head = this->head_;
                    aba_ptr<node> next = head->next;
                    allocator_traits::destroy(this->alloc_, head.get());
                    this->alloc_.deallocate(head, 1);
                    this->head_ = next;
                }
            }

            void push(const T &val)
            {
                aba_ptr<node> n = this->alloc_.allocate(1);
                allocator_traits::construct(this->alloc_, n.get(), aba_ptr<node>(nullptr), val);

                while (1)
                {
                    aba_ptr<node> tail = this->tail_.load();
                    aba_ptr<node> next = tail->next.load();
                    aba_ptr<node> tail2 = this->tail_.load();

                    if (tail == tail2)
                    {
                        if (next == nullptr)
                        {
                            if (tail->next.compare_exchange_weak(next, n))
                            {
                                this->tail_.compare_exchange_strong(tail, n);
                                ++this->size_;
                                this->size_.notify_all();
                                return;
                            }
                        }
                        else
                        {
                            this->tail_.compare_exchange_strong(tail, next);
                        }
                    }
                }
            }

            void pop(T &val)
            {
                while (1)
                {
                    this->size_.wait(0);
                    aba_ptr<node> head = this->head_.load();
                    aba_ptr<node> next = head->next.load();
                    aba_ptr<node> tail = this->tail_.load();
                    aba_ptr<node> head2 = this->head_.load();

                    if (head == head2)
                    {
                        if (head.get() == tail.get())
                        {
                            if (next == nullptr)
                                continue;
                            this->tail_.compare_exchange_strong(tail, next);
                        }
                        else
                        {
                            if (next == nullptr)
                                continue;

                            val = next->data;
                            if (this->head_.compare_exchange_weak(head, next))
                            {
                                allocator_traits::destroy(this->alloc_, head.get());
                                this->alloc_.deallocate(head, 1);
                                --this->size_;
                                return;
                            }
                        }
                    }
                }
            }

            bool empty() const
            {
                return this->size_;
            }

            bool is_lock_free() const
            {
                return this->head_.is_lock_free() && this->tail_.is_lock_free() && this->alloc_.is_lock_free();
            }

            size_type max_size() const
            {
                return std::numeric_limits<std::ptrdiff_t>::max();
            }
        };
    } // namespace parallelism
} // namespace mio