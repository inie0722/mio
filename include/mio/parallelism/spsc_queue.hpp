#pragma once

#include <cstddef>
#include <atomic>
#include <algorithm>
#include <memory>
#include <iterator>

namespace mio
{
    namespace parallelism
    {
        namespace detail
        {
            class base_spsc_queue
            {
            protected:
                using size_type = std::size_t;

                const size_type max_size_;
                std::atomic<size_type> writable_limit_ = 0;
                std::atomic<size_type> readable_limit_ = 0;

                size_type get_index(size_type index) const
                {
                    return index % max_size_;
                }

                base_spsc_queue(size_type max_size)
                    : max_size_(max_size){};

                ~base_spsc_queue() = default;

                template <std::random_access_iterator OutputIt, std::forward_iterator InputIt>
                void push(OutputIt data, InputIt first, size_type count)
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
                        std::copy_n(first, len, data + get_index(writable_limit));
                        std::copy_n(first + len, count - len, data + get_index(writable_limit + len));
                    }
                    else
                    {
                        std::copy_n(first, count, data + get_index(writable_limit));
                    }
                    writable_limit_ += count;
                    writable_limit_.notify_one();
                }

                template <std::random_access_iterator InputIt, std::forward_iterator OutputIt>
                void pop(InputIt data, OutputIt result, size_type count)
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
                        std::copy_n(std::move_iterator(data + get_index(readable_limit)), len, result);
                        std::copy_n(std::move_iterator(data + get_index(readable_limit + len)), count - len, result + len);
                    }
                    else
                    {
                        std::copy_n(std::move_iterator(data + get_index(readable_limit)), count, result);
                    }
                    readable_limit_ += count;
                    readable_limit_.notify_one();
                }

            public:
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
        }
        
        template <typename T, typename Container = std::vector<T>>
        class spsc_queue : public detail::base_spsc_queue
        {
        public:
            using container_type = Container;
            using value_type = typename Container::value_type;
            using size_type = detail::base_spsc_queue::size_type;

        protected:
            container_type c;

            void resize(size_type max_size)
            {
                if constexpr(requires(container_type a){a.resize(max_size);})
                    c.resize(max_size);
            }

        public:
            spsc_queue(size_type max_size)
                : detail::base_spsc_queue(max_size)
            {
                this->resize(max_size);
            }
            
            ~spsc_queue() = default;

            template <typename InputIt>
            void push(InputIt first, size_type count)
            {
                detail::base_spsc_queue::push(this->c.begin(), first, count);
            }

            template <typename OutputIt>
            void pop(OutputIt result, size_type count)
            {
                detail::base_spsc_queue::pop(this->c.begin(), result, count);
            }
        };
    } // namespace parallelism
} // namespace mio