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
                typedef Container_<T_> container_type;

                typedef typename container_type::value_type value_type;
                typedef typename container_type::size_type size_type;
                typedef typename container_type::reference reference;
                typedef typename container_type::const_reference const_reference;

            private:
                alignas(64) container_type c;

                size_t get_index(size_t index) const
                {
                    return index % c.size();
                }

            protected:
                T_ &operator[](size_t index)
                {
                    return this->c[get_index(index)];
                }

                const T_ &operator[](size_t index) const
                {
                    return this->c[get_index(index)];
                }

            public:
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
