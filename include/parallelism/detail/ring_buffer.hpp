#pragma once

#include <stddef.h>
#include <vector>

namespace mio
{
    namespace parallelism
    {
        namespace detail
        {
            template <typename T_, class Container_ = std::vector<T_>>
            class ring_buffer
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
                size_t get_index(size_t index) const
                {
                    return index % c.size();
                }

            public:
                ring_buffer()
                {
                }

                template <typename... Args>
                ring_buffer(Args &&... args) : c(std::forward<Args...>(args)...)
                {
                }

                ring_buffer(const ring_buffer &other)
                {
                    *this = other;
                }

                ring_buffer(ring_buffer &&other)
                {
                    *this = other;
                }

                ring_buffer &operator=(const ring_buffer &other)
                {
                    this->c = other.c;
                }

                ring_buffer &operator=(ring_buffer &&other)
                {
                    this->c = std::move(other.c);
                }

                T_ &operator[](size_t index)
                {
                    return this->c[get_index(index)];
                }

                const T_ &operator[](size_t index) const
                {
                    return this->c[get_index(index)];
                }

                container_type &get_container()
                {
                    return this->c;
                }

                const container_type &get_container() const
                {
                    return this->c;
                }
            };
        } // namespace detail
    }     // namespace parallelism
} // namespace mio
