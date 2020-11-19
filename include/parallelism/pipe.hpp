#pragma once

#include <stddef.h>
#include <string.h>

#include <atomic>
#include <thread>
#include <utility>
#include <vector>
#include <algorithm>

#include "parallelism/wait.hpp"

namespace mio
{
    namespace parallelism
    {
        template <typename T_, typename Container_ = std::vector<T_>>
        class pipe
        {
        public:
            typedef Container_ container_type;

            typedef typename container_type::value_type value_type;
            typedef typename container_type::size_type size_type;
            typedef typename container_type::reference reference;
            typedef typename container_type::const_reference const_reference;

        protected:
            alignas(64) container_type c;

        private:
            alignas(64) std::atomic<size_t> readable_limit_ = 0;
            alignas(64) std::atomic<size_t> writable_limit_ = 0;

            size_t get_index(size_t index) const
            {
                return index % c.size();
            }

            bool is_can_write(size_t count) const
            {
                auto size = c.size();
                return !(readable_limit_ + size < writable_limit_ + count);
            }

            bool is_can_read(size_t count) const
            {
                return !(readable_limit_ + count > writable_limit_);
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
                    std::copy_n(this->c.begin() + get_index(readable_limit_), len, result);
                    std::copy_n(this->c.begin() + get_index(readable_limit_ + len), count - len, result + len);
                }
                else
                {
                    std::copy_n(this->c.begin() + get_index(readable_limit_), count, result);
                }
                readable_limit_ += count;
            }

        public:
            pipe()
            {
            }

            template <typename... Args>
            pipe(Args &&... args) : c(std::forward<Args...>(args)...)
            {
            }

            pipe(const pipe &other)
            {
                *this = other;
            }

            pipe(pipe &&other)
            {
                *this = other;
            }

            pipe &operator=(const pipe &other)
            {
                this->c = other.c;
                this->readable_limit_ = other.readable_limit_.load();
                this->writable_limit_ = other.writable_limit_.load();
            }

            pipe &operator=(pipe &&other)
            {
                this->c = std::move(other.c);

                this->readable_limit_ = other.readable_limit_.load();
                this->writable_limit_ = other.writable_limit_.load();
            }

            container_type &get_container()
            {
                return this->c;
            }

            const container_type &get_container() const
            {
                return this->c;
            }

            template <typename InputIt>
            void write(InputIt first, size_t count, const wait::handler_t &handler = wait::yield)
            {
                //等待可写
                for (size_t i = 0; !is_can_write(count); i++)
                    handler(i);

                __write(first, count);
            }

            template <typename InputIt>
            bool try_write(InputIt first, size_t count)
            {
                //等待可写
                if (!is_can_write(count))
                    return false;

                __write(first, count);
                return true;
            }

            template <typename OutputIt>
            void read(OutputIt result, size_t count, const wait::handler_t &handler = wait::yield)
            {
                //等待可读
                for (size_t i = 0; !is_can_read(count); i++)
                    handler(i);

                __read(result, count);
            }

            template <typename OutputIt>
            bool try_read(OutputIt result, size_t count)
            {
                if (!is_can_read(count))
                    return false;

                __read(result, count);
                return true;
            }

            size_t size() const
            {
                return writable_limit_ - readable_limit_;
            }

            bool empty() const
            {
                return c.empty();
            }

            bool is_lock_free() const
            {
                return true;
            }
        };
    } // namespace parallelism
} // namespace mio