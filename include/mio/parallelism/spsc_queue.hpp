#pragma once

#include <cstddef>
#include <atomic>
#include <array>
#include <algorithm>
#include <memory>

#include <mio/parallelism/utility.hpp>

namespace mio
{
    namespace parallelism
    {
        template <typename T, typename Allocator = std::allocator<T>>
        class spsc_queue
        {
        private:
            using allocator_traits = std::allocator_traits<Allocator>;

        public:
            using value_type = T;

            using allocator_type = typename allocator_traits::allocator_type;

            using size_type = typename allocator_traits::size_type;

            using reference = typename allocator_traits::value_type &;

            using const_reference = const typename allocator_traits::value_type &;

            using pointer = typename allocator_traits::pointer;

            using const_pointer = typename allocator_traits::const_pointer;

        private:
            alignas(hardware_destructive_interference_size) const size_type max_size_;

            alignas(hardware_destructive_interference_size) allocator_type allocator_;

            alignas(hardware_destructive_interference_size) pointer data_;

            alignas(hardware_destructive_interference_size) std::atomic<size_type> writable_limit_ = 0;
            alignas(hardware_destructive_interference_size) std::atomic<size_type> readable_limit_ = 0;

            size_type get_index(size_type index) const
            {
                return index % this->max_size();
            }

        public:
            spsc_queue(size_type max_size)
                : max_size_(max_size)
            {
                this->data_ = allocator_traits::allocate(allocator_, max_size);
            }

            spsc_queue(size_type max_size, const allocator_type &allocator)
                : max_size_(max_size), allocator_(allocator)
            {
                this->data_ = allocator_traits::allocate(allocator_, max_size);
            }

            ~spsc_queue()
            {
                allocator_traits::deallocate(allocator_, this->data_, this->max_size());
            }

            template <typename InputIt>
            void push(InputIt first, size_type count)
            {
            start:
                size_type max_size = this->max_size();
                size_type readable_limit = readable_limit_;
                size_type writable_limit = writable_limit_;

                //等待可写
                if (this->max_size() - (writable_limit - readable_limit) < count)
                {
                    readable_limit_.wait(readable_limit);
                    goto start;
                }

                if ((writable_limit % max_size) + count > max_size)
                {
                    auto len = max_size - (writable_limit % max_size);
                    std::copy_n(first, len, this->data_ + get_index(writable_limit));
                    std::copy_n(first + len, count - len, this->data_ + get_index(writable_limit + len));
                }
                else
                {
                    std::copy_n(first, count, this->data_ + get_index(writable_limit));
                }
                writable_limit_ += count;
                writable_limit_.notify_one();
            }

            template <typename OutputIt>
            void pop(OutputIt result, size_type count)
            {
            start:
                size_type max_size = this->max_size();
                size_type readable_limit = readable_limit_;
                size_type writable_limit = writable_limit_;

                //等待可读
                if (writable_limit - readable_limit < count)
                {
                    writable_limit_.wait(writable_limit);
                    goto start;
                }

                if ((readable_limit % max_size) + count > max_size)
                {
                    auto len = max_size - (readable_limit % max_size);
                    std::copy_n(std::move_iterator(this->data_ + get_index(readable_limit)), len, result);
                    std::copy_n(std::move_iterator(this->data_ + get_index(readable_limit + len)), count - len, result + len);
                }
                else
                {
                    std::copy_n(std::move_iterator(this->data_ + get_index(readable_limit)), count, result);
                }
                readable_limit_ += count;
                readable_limit_.notify_one();
            }

            size_type size() const
            {
                return writable_limit_ - readable_limit_;
            }

            bool empty() const
            {
                return !this->size();
            }

            bool is_lock_free() const
            {
                return writable_limit_.is_lock_free();
            }

            size_type max_size() const
            {
                return max_size_;
            }
        };
    } // namespace parallelism
} // namespace mio