#pragma once

#include "mio/parallelism/utility.hpp"

#include <stddef.h>

#include <atomic>
#include <array>
#include <utility>
#include <algorithm>

namespace mio
{
    namespace parallelism
    {
        template <typename T_, size_t N_ = 4096>
        class pipe
        {
        private:
            alignas(CACHE_LINE) std::array<T_, N_> c;

            alignas(CACHE_LINE) std::atomic<size_t> readable_limit_ = 0;
            alignas(CACHE_LINE) std::atomic<size_t> writable_limit_ = 0;

            size_t get_index(size_t index) const
            {
                return index % c.size();
            }

            template <typename InputIt>
            void __write(InputIt first, size_t count)
            {
                auto size = c.size();

                if ((writable_limit_ % size) + count > size)
                {
                    auto len = size - (writable_limit_ % size);
                    std::copy_n(first, len, this->c.begin() + get_index(writable_limit_));
                    std::copy_n(first + len, count - len, this->c.begin() + get_index(writable_limit_ + len));
                }
                else
                {
                    std::copy_n(first, count, this->c.begin() + get_index(writable_limit_));
                }
                writable_limit_ += count;
            }

            template <typename OutputIt>
            void __read(OutputIt result, size_t count)
            {
                auto size = c.size();

                if ((readable_limit_ % size) + count > size)
                {
                    auto len = size - (readable_limit_ % size);
                    std::copy_n(std::move_iterator(this->c.begin() + get_index(readable_limit_)), len, result);
                    std::copy_n(std::move_iterator(this->c.begin() + get_index(readable_limit_ + len)), count - len, result + len);
                }
                else
                {
                    std::copy_n(std::move_iterator(this->c.begin() + get_index(readable_limit_)), count, result);
                }
                readable_limit_ += count;
            }

        public:
            pipe() = default;

            template <typename InputIt>
            size_t write_some(InputIt first, size_t count, const wait::handler_t &handler = wait::yield)
            {
                size_t size = 0;

                for (size_t i = 0; !size; i++)
                {
                    size = write_size();
                    if (!size)
                        handler(i);
                }

                size = count <= size ? count : size;
                __write(first, size);

                return size;
            }

            template <typename OutputIt>
            size_t read_some(OutputIt result, size_t count, const wait::handler_t &handler = wait::yield)
            {

                size_t size = 0;

                for (size_t i = 0; !size; i++)
                {
                    size = read_size();
                    if (!size)
                        handler(i);
                }

                size = count <= size ? count : size;
                __read(result, size);

                return size;
            }

            template <typename InputIt>
            size_t write(InputIt first, size_t count, const wait::handler_t &handler = wait::yield)
            {
                //等待可写
                for (size_t i = 0; i < count;)
                {
                    size_t size = write_some(first + i, count - i, handler);
                    i += size;
                }

                return count;
            }

            template <typename OutputIt>
            size_t read(OutputIt result, size_t count, const wait::handler_t &handler = wait::yield)
            {
                //等待可读
                for (size_t i = 0; i < count;)
                {
                    size_t size = read_some(result + i, count - i, handler);
                    i += size;
                }

                return count;
            }

            size_t size() const
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

            size_t write_size() const
            {
                return N_ - size();
            }

            size_t read_size() const
            {
                return size();
            }
        };
    } // namespace parallelism
} // namespace mio