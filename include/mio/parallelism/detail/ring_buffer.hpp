#pragma once

#include "mio/parallelism/utility.hpp"

#include <stddef.h>
#include <memory>

namespace mio
{
    namespace parallelism
    {
        namespace detail
        {
            template <typename T, typename Allocator = std::allocator<T>>
            class ring_buffer
            {
            private:
                struct alignas(CACHE_LINE) alignas_t
                {
                    T value;
                };

                using allocator_traits = typename std::allocator_traits<Allocator>::template rebind_traits<alignas_t>;

                using pointer = typename allocator_traits::pointer;

            public:
                using value_type = T;

                using allocator_type = typename allocator_traits::allocator_type;

                using size_type = typename allocator_traits::size_type;

            private:
                alignas(CACHE_LINE) const size_type max_size_;

                allocator_type allocator_;

                alignas(CACHE_LINE) pointer data_;

                size_type get_index(size_type index) const
                {
                    return index % this->max_size();
                }

            public:
                ring_buffer(size_type max_size)
                    : max_size_(max_size)
                {
                    this->data_ = allocator_traits::allocate(allocator_, max_size);
                }

                ~ring_buffer()
                {
                    allocator_traits::deallocate(allocator_, this->data_, this->max_size());
                }

                value_type &operator[](size_type index)
                {
                    return this->data_[get_index(index)].value;
                }

                const value_type &operator[](size_type index) const
                {
                    return this->data_[get_index(index)].value;
                }

                size_type max_size() const
                {
                    return this->max_size_;
                }
            };
        } // namespace detail
    }     // namespace parallelism
} // namespace mio
