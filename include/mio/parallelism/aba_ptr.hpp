#pragma once

#include <stddef.h>
#include <stdint.h>
#include <atomic>

namespace mio
{
    namespace parallelism
    {
        template <typename T>
        class aba_ptr
        {
        private:
            friend class std::atomic<aba_ptr<T>>;

            template <typename>
            friend class aba_ptr;

            static constexpr uint64_t COUNT_MASK = 0XFFFFull << 48;

            static constexpr uint64_t POINTER_MASK = ~COUNT_MASK;

            aba_ptr(uint64_t ptr)
            {
                ptr_ = ptr;
            }

        public:
            uint64_t ptr_;

            aba_ptr() = default;

            aba_ptr(T *ptr)
            {
                *this = ptr;
            }

            aba_ptr(const aba_ptr &ptr)
            {
                ptr_ = ptr.ptr_;
            }

            ~aba_ptr() = default;

            aba_ptr &operator=(T *ptr)
            {
                ptr_ &= COUNT_MASK;
                ptr_ += (uint64_t)ptr;

                return *this;
            }

            aba_ptr &operator=(const aba_ptr &ptr)
            {
                ptr_ = ptr.ptr_;
                return *this;
            }

            T *get() const
            {
                return reinterpret_cast<T *>(ptr_ & POINTER_MASK);
            }

            T &operator*() const
            {
                return *get();
            }

            T *operator->() const
            {
                return get();
            }

            bool operator==(const aba_ptr &ptr) const
            {
                return this->ptr_ == ptr.ptr_;
            }

            bool operator!=(const aba_ptr &ptr) const
            {
                return this->ptr_ != ptr.ptr_;
            }

            bool operator!=(std::nullptr_t) const
            {
                return this->get() != nullptr;
            }

            bool operator==(std::nullptr_t) const
            {
                return this->get() == nullptr;
            }

            template <typename AIMS>
            explicit operator aba_ptr<AIMS>() const
            {
                return aba_ptr<AIMS>(ptr_);
            }
        };
    } // namespace parallelism
} // namespace mio

namespace std
{
    template <typename T>
    class atomic<mio::parallelism::aba_ptr<T>>
    {
    public:
        using aba_ptr = mio::parallelism::aba_ptr<T>;

    private:
        static constexpr uint64_t COUNT_MASK = aba_ptr::COUNT_MASK;
        static constexpr uint64_t POINTER_MASK = aba_ptr::POINTER_MASK;
        std::atomic<uint64_t> ptr_;

    public:
        atomic() = default;

        atomic(const aba_ptr &ptr)
        {
            ptr_ = ptr.ptr_;
        }

        atomic &operator=(aba_ptr desired)
        {
            store(desired);
            return *this;
        }

        operator aba_ptr() const
        {
            return load();
        }

        aba_ptr load(std::memory_order failure = std::memory_order_seq_cst) const
        {
            return aba_ptr(ptr_.load(failure));
        }

        void store(aba_ptr desired, std::memory_order failure = std::memory_order_seq_cst)
        {
            uint16_t count = ((desired.ptr_ & COUNT_MASK) >> 48) + 1;
            uint64_t des = ((uint64_t)count << 48) + (desired.ptr_ & POINTER_MASK);

            ptr_.store(des, failure);
        }

        aba_ptr exchange(aba_ptr desired, std::memory_order failure = std::memory_order_seq_cst)
        {
            uint16_t count = ((desired.ptr_ & COUNT_MASK) >> 48) + 1;
            uint64_t des = ((uint64_t)count << 48) + (desired.ptr_ & POINTER_MASK);

            return aba_ptr(ptr_.exchange(des, failure));
        }

        bool compare_exchange_weak(aba_ptr &expected, const aba_ptr &desired, std::memory_order failure = std::memory_order_seq_cst)
        {
            uint16_t count = ((expected.ptr_ & COUNT_MASK) >> 48) + 1;

            uint64_t des = ((uint64_t)count << 48) + (desired.ptr_ & POINTER_MASK);

            return ptr_.compare_exchange_weak(expected.ptr_, des, failure);
        }

        bool compare_exchange_strong(aba_ptr &expected, const aba_ptr &desired, std::memory_order failure = std::memory_order_seq_cst)
        {
            uint16_t count = ((expected.ptr_ & COUNT_MASK) >> 48) + 1;

            uint64_t des = ((uint64_t)count << 48) + (desired.ptr_ & POINTER_MASK);
            return ptr_.compare_exchange_strong(expected.ptr_, des, failure);
        }

        aba_ptr fetch_add(ptrdiff_t arg, std::memory_order failure = std::memory_order_seq_cst)
        {
            uint16_t count = ((expected.ptr_ & COUNT_MASK) >> 48) + 1;

            uint64_t des = ((uint64_t)count << 48) + (arg * sizeof(T) & POINTER_MASK);

            return aba_ptr(ptr_.fetch_add(des, failure));
        }

        aba_ptr fetch_sub(ptrdiff_t arg, std::memory_order failure = std::memory_order_seq_cst)
        {
            uint16_t count = ((expected.ptr_ & COUNT_MASK) >> 48) + 1;

            uint64_t des = ((uint64_t)count << 48) + (arg * sizeof(T) & POINTER_MASK);
            return aba_ptr(ptr_.fetch_sub(des, failure));
        }
    };
} // namespace std