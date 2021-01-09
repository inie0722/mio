#pragma once

#include "mio/parallelism/utility.hpp"

#include <stddef.h>

#include <atomic>
#include <array>
#include <utility>
#include <algorithm>
#include <memory>

namespace mio
{
    namespace parallelism
    {
        template <typename T, typename Allocator = std::allocator<T>>
        class pipe
        {
        private:
            using allocator_traits = std::allocator_traits<Allocator>;

        public:
            using value_type = typename allocator_traits::value_type;

            using allocator_type = typename allocator_traits::allocator_type;

            using size_type = typename allocator_traits::size_type;

            using reference = typename allocator_traits::value_type &;

            using const_reference = const typename allocator_traits::value_type &;

            using pointer = typename allocator_traits::pointer;

            using const_pointer = typename allocator_traits::const_pointer;

        private:
            alignas(CACHE_LINE) const size_type max_size_;

            allocator_type allocator_;

            alignas(CACHE_LINE) pointer data_;

            alignas(CACHE_LINE) std::atomic<size_type> writable_limit_;
            alignas(CACHE_LINE) std::atomic<size_type> readable_limit_;

            size_type get_index(size_type index) const
            {
                return index % this->max_size();
            }

            template <typename InputIt>
            void __write(InputIt first, size_type count)
            {
                auto max_size = this->max_size();

                if ((writable_limit_ % max_size) + count > max_size)
                {
                    auto len = max_size - (writable_limit_ % max_size);
                    std::copy_n(first, len, this->data_ + get_index(writable_limit_));
                    std::copy_n(first + len, count - len, this->data_ + get_index(writable_limit_ + len));
                }
                else
                {
                    std::copy_n(first, count, this->data_ + get_index(writable_limit_));
                }
                writable_limit_ += count;
            }

            template <typename OutputIt>
            void __read(OutputIt result, size_type count)
            {
                auto max_size = this->max_size();

                if ((readable_limit_ % max_size) + count > max_size)
                {
                    auto len = max_size - (readable_limit_ % max_size);
                    std::copy_n(std::move_iterator(this->data_ + get_index(readable_limit_)), len, result);
                    std::copy_n(std::move_iterator(this->data_ + get_index(readable_limit_ + len)), count - len, result + len);
                }
                else
                {
                    std::copy_n(std::move_iterator(this->data_ + get_index(readable_limit_)), count, result);
                }
                readable_limit_ += count;
            }

        public:
            pipe(size_type max_size)
                : max_size_(max_size), writable_limit_(0), readable_limit_(0)
            {
                this->data_ = allocator_traits::allocate(allocator_, max_size);
            }

            pipe(size_type max_size, const allocator_type &allocator)
                : max_size_(max_size), allocator_(allocator), writable_limit_(0), readable_limit_(0)
            {
                this->data_ = allocator_traits::allocate(allocator_, max_size);
            }

            ~pipe()
            {
                allocator_traits::deallocate(allocator_, this->data_, this->max_size());
            }

            template <typename InputIt>
            size_type write_some(InputIt first, size_type count, const wait::handler_t &handler = wait::yield)
            {
                size_type size = 0;

                for (size_type i = 0; !size; i++)
                {
                    size = this->max_size() - this->size();
                    if (!size)
                        handler(i);
                }

                size = count <= size ? count : size;
                this->__write(first, size);

                return size;
            }

            template <typename OutputIt>
            size_type read_some(OutputIt result, size_type count, const wait::handler_t &handler = wait::yield)
            {

                size_type size = 0;

                for (size_type i = 0; !size; i++)
                {
                    size = this->size();
                    if (!size)
                        handler(i);
                }

                size = count <= size ? count : size;
                this->__read(result, size);

                return size;
            }

            template <typename InputIt>
            size_type write(InputIt first, size_type count, const wait::handler_t &handler = wait::yield)
            {
                //等待可写
                for (size_type i = 0; this->max_size() - this->size() < count; i++)
                {
                    handler(i);
                }

                this->__write(first, count);
                return count;
            }

            template <typename OutputIt>
            size_type read(OutputIt result, size_type count, const wait::handler_t &handler = wait::yield)
            {
                //等待可读
                for (size_type i = 0; this->size() < count; i++)
                {
                    handler(i);
                }

                this->__read(result, count);
                return count;
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
                return true;
            }

            size_type max_size() const
            {
                return max_size_;
            }
        };
    } // namespace parallelism
} // namespace mio