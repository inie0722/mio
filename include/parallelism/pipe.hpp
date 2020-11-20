#pragma once

#include <stddef.h>
#include <string.h>

#include <atomic>
#include <thread>
#include <utility>
#include <vector>
#include <algorithm>
#include <tuple>
#include <functional>
#include <limits>

#include "parallelism/utility.hpp"

namespace mio
{
    namespace parallelism
    {
        template <typename T_, size_t Extent, template <typename> typename Container_>
        class pipe
        {
        public:
            template <typename T__>
            using container_type = Container_<T__>;

            template <typename T__>
            using value_type = typename container_type<T__>::value_type;

            template <typename T__>
            using size_type = typename container_type<T__>::size_type;

            template <typename T__>
            using reference = typename container_type<T__>::reference;

            template <typename T__>
            using const_reference = typename container_type<T__>::const_reference;

        protected:
            alignas(CACHE_LINE) container_type<T_> c;

        private:
            alignas(CACHE_LINE) std::atomic<size_t> readable_limit_ = 0;
            alignas(CACHE_LINE) std::atomic<size_t> writable_limit_ = 0;

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
            pipe() = default;

            template <typename... Args>
            pipe(size_t size, Args &&... args) : c(std::forward<Args...>(args)...)
            {
                if constexpr(Extent == dynamic_extent)
                {
                    c.resize(size);
                }
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

            void resize(size_t count)
            {
                if constexpr(Extent == std::numeric_limits<std::size_t>::max())
                {
                    c.resize(count);
                }
            }

            bool empty() const
            {
                return !this->size();
            }

            bool is_lock_free() const
            {
                return true;
            }

            auto get_container()
            {
                return std::make_tuple(std::ref(this->c));
            }

            const auto get_container() const
            {
                return std::make_tuple(std::cref(this->c));
            }
        };
    } // namespace parallelism
} // namespace mio