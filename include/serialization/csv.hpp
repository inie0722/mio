#pragma once

#include <string>
#include <unordered_map>
#include <tuple>
#include <vector>
#include <fstream>
#include <utility>
#include <type_traits>
#include <stddef.h>
#include <array>
#include <initializer_list>

#include <boost/algorithm/string.hpp>

namespace serialization
{
    template <typename... Args>
    class CSV
    {
    private:
        std::vector<std::tuple<Args...>> data;
        std::unordered_map<std::string, ptrdiff_t> index_map;

        template <size_t... ints>
        static auto get_index(std::index_sequence<ints...> int_seq)
        {

            std::tuple<Args...> a{};
            
            std::array<ptrdiff_t, sizeof...(Args)> index{};
        
            ((index[ints] = (char *)&std::get<ints>(a) - (char *)&std::get<0>(a)), ...);
            return index;
        }

        inline static std::array<ptrdiff_t, sizeof...(Args)> index_array{get_index(std::make_index_sequence<sizeof...(Args)>{})};

    public:
        template <typename T>
        class Iterator
        {
        private: 
            std::vector<std::tuple<Args...>> *data;
            size_t index;
            ptrdiff_t offset;
        public:
            Iterator(std::vector<std::tuple<Args...>> &data, size_t index, ptrdiff_t offset) : data(&data), index(index), offset(offset)
            {
            }

            Iterator(const Iterator & other)
            {
                *this = other;
            }

            Iterator& operator = (const Iterator & other)
            {
                this->data = other.data;
                this->index = other.index;
                this->offset = other.offset;

                return *this;
            }

            T & operator*()
            {
                return *(T *)((char *)&std::get<0>((*data)[index]) + offset);
            }

            T &operator->()
            {
                return *(T *)((char *)&std::get<0>((*data)[index]) + offset);
            }

            T & operator[](size_t index)
            {
                return *(T *)((char *)&std::get<0>((*data)[this->index + index]) + offset);
            }

            bool operator==(const Iterator & a)
            {
                return this->data == a.data && this->index == a.index && this->offset == a.offset;
            }

            bool operator!=(const Iterator &a)
            {
                return !(*this == a);
            }

            Iterator &operator+=(int i)
            {
                index += i;
                return *this;
            }

            Iterator &operator-=(int i)
            {
                index -= i;
                return *this;
            }

            Iterator & operator++()
            {
                ++index;
                return *this;
            }

            Iterator operator++(int)
            {
                Iterator temp = *this;
                ++index;
                return temp;
            }

            Iterator & operator--()
            {
                --index;
                return *this;
            }

            Iterator operator--(int)
            {
                Iterator temp = *this;
                --index;
                return temp;
            }
        };

        CSV()
        {}

        CSV(const std::string &file_name)
        {
            std::ifstream file(file_name);
            std::string line;
            getline(file, line);

            std::vector<std::string> head;
            split(head, line, boost::is_any_of(","));
            
            if (head.size() != sizeof...(Args))
            {
                assert(0);
            }
            else
            {
                for (size_t i = 0; i < index_array.size(); i++)
                {
                    index_map[head[i]] = index_array[i];
                }
            }

            while (getline(file, line))
            {
                std::vector<std::string> item;
                split(item, line, boost::is_any_of(","));

                size_t index = 0;
                std::tuple<Args...> tup{};

                auto f = [&](auto a) {
                    decltype(a) *p = (decltype(a) *)((char *)&std::get<0>(tup) + index_array[index]);
                    if constexpr (std::is_integral<decltype(a)>::value && std::is_signed<decltype(a)>::value)
                    {
                        *p = (decltype(a))std::stoll(item[index]);
                    }
                    else if constexpr (std::is_integral<decltype(a)>::value && std::is_unsigned<decltype(a)>::value)
                    {
                        *p = (decltype(a))std::stoull(item[index]);
                    }
                    else if constexpr (std::is_floating_point<decltype(a)>::value)
                    {
                        *p = (decltype(a))std::stold(item[index]);
                    }
                    else
                    {
                        *p = item[index];
                    }
                    ++index;
                };

                (f(Args()), ...);

                data.push_back(tup);
            }
        }

        CSV(const CSV &other)
        {
            *this = other;
        }

        CSV & operator=(const CSV &other)
        {
            this->data = other.data;
            this->index_map = other.index_map;
            
            return *this;
        }

        template <typename T>
        Iterator<T> begin(const std::string &column)
        {
            return Iterator<T>(data, 0, index_map.at(column));
        }

        template <typename T>
        Iterator<T> end(const std::string &column)
        {
            return Iterator<T>(data, data.size(), index_map.at(column));
        }
    };
} // namespace serialization
