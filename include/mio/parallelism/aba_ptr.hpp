#pragma once

#include <cstddef>
#include <cstdint>
#include <atomic>

static_assert(sizeof(void *) == 8, "64-bit system only");

namespace mio
{
    namespace parallelism
    {
        template <typename T>
        class aba_ptr
        {
        public:
            using element_type = T;
            using pointer = element_type *;

        private:
            friend class std::atomic<aba_ptr<T>>;

            template <typename>
            friend class aba_ptr;

            static constexpr std::uint64_t COUNT_MASK = 0XFFFFull << 48;

            static constexpr std::uint64_t POINTER_MASK = ~COUNT_MASK;

            std::uint64_t ptr_;

            aba_ptr(std::uint64_t ptr)
            {
                ptr_ = ptr;
            }

        public:
            aba_ptr() = default;
            ~aba_ptr() = default;

            aba_ptr(pointer ptr) noexcept
            {
                *this = ptr;
            }

            aba_ptr(const aba_ptr &ptr) noexcept
            {
                ptr_ = ptr.ptr_;
            }

            aba_ptr &operator=(pointer ptr) noexcept
            {
                ptr_ &= COUNT_MASK;
                ptr_ += (std::uint64_t)ptr;

                return *this;
            }

            aba_ptr &operator=(const aba_ptr &ptr) noexcept
            {
                ptr_ = ptr.ptr_;
                return *this;
            }

            pointer get() const volatile noexcept
            {
                return reinterpret_cast<pointer>(ptr_ & POINTER_MASK);
            }

            element_type &operator*() const volatile noexcept
            {
                return *get();
            }

            pointer operator->() const volatile noexcept
            {
                return get();
            }

            element_type operator[](std::size_t i) const volatile noexcept
            {
                return get()[i];
            }

            bool operator==(const aba_ptr &ptr) const volatile noexcept
            {
                return this->ptr_ == ptr.ptr_;
            }

            bool operator==(std::nullptr_t) const volatile noexcept
            {
                return this->get() == nullptr;
            }

            template <typename AIMS>
            explicit operator aba_ptr<AIMS>() const volatile noexcept
            {
                return aba_ptr<AIMS>(ptr_);
            }

            operator aba_ptr<const element_type>() const volatile noexcept
            {
                return aba_ptr<const element_type>(ptr_);
            }

            explicit operator bool() const volatile noexcept
            {
                return this->get() != nullptr;
            }

            static aba_ptr pointer_to(element_type &r) noexcept
            {
                return aba_ptr(r);
            }

            static element_type *to_address(aba_ptr p) noexcept
            {
                return p.get();
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
        using value_type = mio::parallelism::aba_ptr<T>;
        static constexpr bool is_always_lock_free = std::atomic<std::uint64_t>::is_always_lock_free;

    private:
        using aba_ptr = mio::parallelism::aba_ptr<T>;
        static constexpr std::uint64_t COUNT_MASK = aba_ptr::COUNT_MASK;
        static constexpr std::uint64_t POINTER_MASK = aba_ptr::POINTER_MASK;
        std::atomic<std::uint64_t> ptr_;

    public:
        atomic() = default;

        ~atomic() = default;

        atomic(const atomic &) = delete;

        atomic(const aba_ptr &desired) noexcept
        {
            ptr_ = desired.ptr_;
        }

        aba_ptr operator=(aba_ptr desired) volatile noexcept
        {
            store(desired);
            return desired;
        }

        atomic &operator=(const atomic &) volatile = delete;

        bool is_lock_free() const volatile noexcept
        {
            return ptr_.is_lock_free();
        }

        void store(aba_ptr desired, std::memory_order order = std::memory_order_seq_cst) volatile noexcept
        {
            uint16_t count = ((desired.ptr_ & COUNT_MASK) >> 48) + 1;
            std::uint64_t des = ((std::uint64_t)count << 48) + (desired.ptr_ & POINTER_MASK);

            ptr_.store(des, order);
        }

        aba_ptr load(std::memory_order order = std::memory_order_seq_cst) const volatile noexcept
        {
            return aba_ptr(ptr_.load(order));
        }

        operator aba_ptr() const volatile noexcept
        {
            return load();
        }

        aba_ptr exchange(aba_ptr desired, std::memory_order order = std::memory_order_seq_cst) volatile noexcept
        {
            uint16_t count = ((desired.ptr_ & COUNT_MASK) >> 48) + 1;
            std::uint64_t des = ((std::uint64_t)count << 48) + (desired.ptr_ & POINTER_MASK);

            return aba_ptr(ptr_.exchange(des, order));
        }

        bool compare_exchange_weak(aba_ptr &expected, aba_ptr desired, std::memory_order order = std::memory_order_seq_cst) volatile noexcept
        {
            uint16_t count = ((expected.ptr_ & COUNT_MASK) >> 48) + 1;

            std::uint64_t des = ((std::uint64_t)count << 48) + (desired.ptr_ & POINTER_MASK);

            return ptr_.compare_exchange_weak(expected.ptr_, des, order);
        }

        bool compare_exchange_strong(aba_ptr &expected, aba_ptr desired, std::memory_order order = std::memory_order_seq_cst) volatile noexcept
        {
            uint16_t count = ((expected.ptr_ & COUNT_MASK) >> 48) + 1;

            std::uint64_t des = ((std::uint64_t)count << 48) + (desired.ptr_ & POINTER_MASK);
            return ptr_.compare_exchange_strong(expected.ptr_, des, order);
        }

        void wait(aba_ptr old, std::memory_order order = std::memory_order::seq_cst) const volatile noexcept
        {
            ptr_.wait(old.ptr_, order);
        }

        void notify_one() noexcept
        {
            ptr_.notify_one();
        }

        void notify_all() noexcept
        {
            ptr_.notify_all();
        }

        aba_ptr fetch_add(ptrdiff_t arg, std::memory_order order = std::memory_order_seq_cst) volatile noexcept
        {
            std::uint64_t des = (1ull << 48) + (arg * sizeof(T) & POINTER_MASK);

            return aba_ptr(ptr_.fetch_add(des, order));
        }

        aba_ptr fetch_sub(ptrdiff_t arg, std::memory_order order = std::memory_order_seq_cst) volatile noexcept
        {
            std::uint64_t des = (1ull << 48) + (arg * sizeof(T) & POINTER_MASK);
            return aba_ptr(ptr_.fetch_sub(des, order));
        }
    };
} // namespace std