#include <mio/metaprogram/hash.hpp>
#include <initializer_list>
#include <optional>
#include <iterator>
#include <stddef.h>
#include <stdint.h>
#include <array>
#include <iostream>

namespace mio
{
    namespace metaprogram
    {
        template <typename Key, typename T, size_t N, class Hash = hash<Key>, class KeyEqual = std::equal_to<Key>>
        class unordered_map
        {
        public:
            class iterator;

            class const_iterator;

            using key_type = Key;

            using value_type = std::pair<const Key, T>;

            using size_type = size_t;

            using difference_type = ptrdiff_t;

            using hasher = Hash;

            using key_equal = KeyEqual;

            using reference = value_type &;

            using const_reference = const value_type &;

            using pointer = value_type *;

            using const_pointer = const value_type *;

            using reverse_iterator = std::reverse_iterator<iterator>;

            using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        private:
            friend iterator;

            using base_container_t = typename std::array<std::pair<bool, std::pair<Key, T>>, N>;

            base_container_t data_;

            Hash hash_;

            KeyEqual key_equal_;

            size_t size_;

        public:
            class iterator
            {
            public:
                using iterator_category = std::bidirectional_iterator_tag;

                using value_type = Key;

                using reference = value_type &;

                using pointer = value_type *;

                using difference_type = ptrdiff_t;

            private:
                const unordered_map *set_;

                decltype(std::declval<base_container_t>().begin()) cur_;

            public:
                constexpr iterator(const unordered_map *set, decltype(cur_) cur) : set_(set), cur_(cur){};

                constexpr bool operator==(iterator a) const
                {
                    return a.set_ == this->set_ && a.cur_ == this->cur_;
                }

                constexpr bool operator!=(iterator a) const
                {
                    return !(*this == a);
                }

                constexpr iterator operator++()
                {
                    while (cur_ != set_->data_.end())
                    {
                        ++cur_;

                        if (cur_ != set_->data_.end() && cur_->has_value())
                        {
                            break;
                        }
                    }

                    return *this;
                }

                constexpr iterator operator++(int)
                {
                    iterator tmp = *this;
                    ++(*this);
                    return tmp;
                }

                constexpr iterator operator--()
                {
                    while (1)
                    {
                        if (cur_ != set_->data_.begin())
                        {
                            --cur_;

                            if (cur_ != set_->data_.begin() && cur_->has_value())
                                break;
                        }
                    }

                    return *this;
                }

                constexpr iterator operator--(int)
                {
                    iterator tmp = *this;
                    --(*this);
                    return tmp;
                }

                constexpr reference &operator*() const
                {
                    return **cur_;
                }
            };

            class const_iterator
            {
            public:
                using iterator_category = std::bidirectional_iterator_tag;

                using value_type = Key;

                using reference = const value_type &;

                using pointer = const value_type *;

                using difference_type = ptrdiff_t;

            private:
                const unordered_map *set_;

                decltype(std::declval<const base_container_t>().begin()) cur_;

            public:
                constexpr const_iterator(const unordered_map *set, decltype(cur_) cur) : set_(set), cur_(cur){};

                constexpr bool operator==(iterator a)
                {
                    return a.set_ == this->cur_ && a.cur_ == this->cur_;
                }

                constexpr bool operator!=(iterator a) const
                {
                    return !(*this == a);
                }

                constexpr const_iterator operator++()
                {
                    while (1)
                    {
                        if (cur_ != set_->data_.end())
                        {
                            ++cur_;

                            if (cur_->has_value())
                                break;
                        }
                    }

                    return *this;
                }

                constexpr const_iterator operator++(int)
                {
                    const_iterator tmp = *this;
                    ++(*this);
                    return tmp;
                }

                constexpr const_iterator operator--()
                {
                    while (1)
                    {
                        if (cur_ != set_->data_.begin())
                        {
                            --cur_;

                            if (cur_->has_value())
                                break;
                        }
                    }

                    return *this;
                }

                constexpr const_iterator operator--(int)
                {
                    iterator tmp = *this;
                    --(*this);
                    return tmp;
                }

                constexpr auto &operator*() const
                {
                    return (*cur_).second;
                }
            };

            constexpr unordered_map( std::initializer_list<value_type> val_list)
            {
                if (val_list.size() > N)
                {
                    throw std::runtime_error("Too many values");
                }

                this->size_ = val_list.size();

                for (auto val : val_list)
                {
                    auto hash_code = hash_(val.first);
                    auto index = hash_code % N;
                    for (size_t i = 0; i < N; i++)
                    {
                        index = (index + i) % N;
                        if (!data_[index].first)
                        {
                            data_[index].second =  value_type (val);
                            data_[index].first = true;
                            break;
                        }
                        else
                        {
                            //不允许 键入重复值
                            if (data_[index].second.first == val.first)
                            {
                                throw std::runtime_error("Do not allow duplicate keys to be entered");
                            }
                        }
                    }
                }
            }

            constexpr iterator begin()
            {
                return iterator(this, this->data_.begin());
            }

            constexpr iterator end()
            {
                return iterator(this, this->data_.end());
            }

            constexpr const_iterator begin() const
            {
                return const_iterator(this, this->data_.begin());
            }

            constexpr const_iterator end() const
            {
                return const_iterator(this, this->data_.end());
            }

            constexpr reverse_iterator rbegin()
            {
                return std::reverse_iterator<iterator>(iterator(this, this->data_.begin().base()));
            }

            constexpr reverse_iterator rend()
            {
                return std::reverse_iterator<iterator>(iterator(this, this->data_.rend().base()));
            }

            constexpr const_reverse_iterator rbegin() const
            {
                return std::reverse_iterator<const_iterator>(const_iterator(this, this->data_.rbegin().base()));
            }

            constexpr const_reverse_iterator rend() const
            {
                return std::reverse_iterator<const_iterator>(const_iterator(this, this->data_.rend().base()));
            }

            constexpr iterator find(Key val)
            {
                auto index = __find(val);
                return index != N ? iterator(this, &this->data_[index]) : this->end();
            }

            constexpr const_iterator find(Key val) const
            {
                auto index = __find(val);
                
                return index != N ? const_iterator(this, &this->data_[index]) : this->end();
            }

            constexpr size_t size() const
            {
                return N;
            }

            constexpr size_t max_size() const
            {
                return N;
            }

        private:
            constexpr size_t __find(Key val) const
            {
                auto hash_code = hash_(val);
                auto index = hash_code % N;

                for (size_t i = 0; i < N; i++)
                {
                    index = (index + i) % N;
                    if (data_[index].first)
                    {
                        if (data_[index].second.first == val)
                        {
                            return index;
                        }
                    }
                    else
                    {
                        break;
                    }
                }

                return N;
            }
        };
    }
}