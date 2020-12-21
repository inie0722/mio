#pragma once

#include "mio/parallelism/utility.hpp"

#include <stddef.h>

#include <array>

namespace mio
{
    namespace parallelism
    {
        namespace detail
        {
            template <typename T_, size_t N_ = 4096>
            class ring_buffer
            {
            private:
                struct alignas(CACHE_LINE) alignas_t
                {
                    T_ value;
                };

                alignas(CACHE_LINE) std::array<alignas_t, N_> c;

                size_t get_index(size_t index) const
                {
                    return index % c.size();
                }

            public:
                ring_buffer() = default;

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
                    return *this;
                }

                ring_buffer &operator=(ring_buffer &&other)
                {
                    this->c = std::move(other.c);
                    return *this;
                }

                T_ &operator[](size_t index)
                {
                    return this->c[get_index(index)].value;
                }

                const T_ &operator[](size_t index) const
                {
                    return this->c[get_index(index)].value;
                }

                size_t size() const
                {
                    return c.size();
                }
            };
        } // namespace detail
    }     // namespace parallelism
} // namespace mio
