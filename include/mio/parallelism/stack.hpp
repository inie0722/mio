/**
 * @file stack.hpp
 * @author 然Y (inie0722@gmail.com)
 * @brief
 * @version 0.1
 * @date 2022-03-07
 *
 * @copyright Copyright (c) 2022
 *
 */
#pragma once

#include <atomic>
#include <limits>
#include <cstddef>
#include <memory>

#include <mio/parallelism/aba_ptr.hpp>
#include <mio/parallelism/allocator.hpp>

namespace mio
{
    /// @brief 并行
    namespace parallelism
    {
        /**
         * @brief 无锁无界栈
         *
         * @tparam T
         * @tparam Allocator
         */
        template <typename T, typename Allocator = std::allocator<T>>
        class stack
        {
        public:
            using value_type = T;
            using size_type = std::size_t;

        private:
            struct node
            {
                std::atomic<aba_ptr<node>> next;
                value_type value;
            };

            using allocator_type = typename mio::parallelism::allocator<node, typename std::allocator_traits<Allocator>::rebind_alloc<node>>;
            using allocator_traits = std::allocator_traits<allocator_type>;

            allocator_type alloc_;
            std::atomic<aba_ptr<node>> top_ = aba_ptr<node>(nullptr);

        public:
            stack() = default;

            ~stack()
            {
                while (this->top_.load())
                {
                    aba_ptr<node> top = this->top_;
                    aba_ptr<node> next = top->next;
                    allocator_traits::destroy(this->alloc_, top.get());
                    this->alloc_.deallocate(this->top_, 1);
                    this->top_ = next;
                }
            }

            void push(const value_type &val)
            {
                aba_ptr<node> exp = this->top_;
                aba_ptr<node> n = this->alloc_.allocate(1);
                allocator_traits::construct(this->alloc_, n.get(), exp, val);

                while (!this->top_.compare_exchange_weak(exp, n))
                {
                    n->next = exp;
                }
                this->top_.notify_all();
            }

            void pop(value_type &val)
            {
                aba_ptr<node> exp = this->top_;
                aba_ptr<node> ret = exp;

                while (!exp || !this->top_.compare_exchange_weak(exp, exp->next))
                {
                    while (!exp)
                    {
                        this->top_.wait(exp);
                        exp = this->top_;
                    }

                    ret = exp;
                }

                val = std::move(ret->value);

                allocator_traits::destroy(this->alloc_, exp.get());
                this->alloc_.deallocate(exp, 1);
            }

            bool empty() const
            {
                return this->top_ == nullptr;
            }

            bool is_lock_free() const
            {
                return this->top_.is_lock_free() && this->alloc_.is_lock_free();
            }

            size_type max_size() const
            {
                return std::numeric_limits<std::ptrdiff_t>::max();
            }
        };

    } // namespace parallelism
} // namespace mio