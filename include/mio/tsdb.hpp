#pragma once

#include <atomic>
#include <stdint.h>
#include <type_traits>
#include <fstream>
#include <memory>
#include <optional>

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

namespace mio
{
    namespace tsdb
    {
        template <typename T>
        class row
        {
        public:
            using value_type = T;

        private:
            std::atomic<bool> is_write_;
            value_type value_;

        public:
            row &operator=(const value_type &val)
            {
                value_ = val;
                is_write_ = true;
                return *this;
            }

            value_type &operator*()
            {
                return value_;
            }

            const value_type &operator*() const
            {
                return value_;
            }

            value_type *operator->()
            {
                return &value_;
            }

            const value_type *operator->() const
            {
                return &value_;
            }

            bool has_value() const
            {
                return is_write_;
            }

            operator bool() const
            {
                return has_value();
            }

            void wait() const
            {
                while (!is_write_)
                    ;
            }

            value_type &value()
            {
                if (!is_write_)
                {
                    throw std::bad_optional_access();
                }

                return value_;
            }

            const value_type &value() const
            {
                return const_cast<row *>(this)->value();
            }

            template <typename U>
            value_type value_or(U &&default_value) const
            {
                if (!is_write_)
                {
                    return std::move(default_value);
                }
                else
                {
                    return value_;
                }
            }
        };

        template <typename T>
        class table
        {
        public:
            using value_type = T;
            using row_type = row<value_type>;

        private:
            struct header
            {
                std::atomic<uint64_t> size;
                std::atomic<uint64_t> capacity;
                std::atomic<uint64_t> ref_cout;
                std::atomic_flag lock;
            };

            std::string mmap_name_;
            std::unique_ptr<boost::interprocess::file_mapping> file_mapp_;
            std::unique_ptr<boost::interprocess::mapped_region> region_;

            header *header_;
            row_type *row_;

            //本地 capacity
            uint64_t capacity_;

            void create_file(size_t size)
            {
                std::filebuf fbuf;
                fbuf.open(mmap_name_, std::ios::in | std::ios::out | std::ios::trunc | std::ios::binary);
                fbuf.pubseekoff(size - 1, std::ios::beg);
                fbuf.sputc(0);
            }

            //设置文件大小
            void file_resize(size_t size)
            {
                std::filebuf fbuf;
                fbuf.open(mmap_name_, std::ios::in | std::ios::out | std::ios::binary);
                fbuf.pubseekoff(size - 1, std::ios::beg);
                fbuf.sputc(0);
            }

            void recapacity()
            {
                file_resize(sizeof(header) + header_->capacity * 2 * sizeof(row_type));
                header_->capacity = header_->capacity * 2;
            }

            //重新映射
            void remmap()
            {
                using namespace boost::interprocess;
                region_->~mapped_region();
                new (region_.get()) boost::interprocess::mapped_region(*file_mapp_, read_write);
                header_ = static_cast<header *>(region_->get_address());

                row_ = (row_type *)((char *)region_->get_address() + sizeof(header));
                this->capacity_ = get_region_capacity();
            }

            //获取现在映射的内存的 capacity
            size_t get_region_capacity() const
            {
                return (this->region_->get_size() - sizeof(header)) / sizeof(row_type);
            }

            size_t do_push(const value_type &val, size_t index)
            {
                if (index >= this->capacity_)
                {
                    auto flag = header_->lock.test_and_set();

                    if (!flag)
                    {
                        recapacity();
                        header_->lock.clear();
                    }

                    while (this->capacity_ == header_->capacity)
                        ;

                    remmap();

                    return do_push(val, index);
                }

                row_[index] = val;

                return index;
            }

            row_type &do_read(size_t index)
            {
                if (index >= this->capacity_)
                {
                    auto flag = header_->lock.test_and_set();
                    if (!flag)
                    {
                        recapacity();
                        header_->lock.clear();
                    }

                    while (this->capacity_ == header_->capacity)
                        ;

                    remmap();

                    return do_read(index);
                }
                return row_[index];
            }

        public:
            table(const std::string &name, size_t capacity)
            {
                using namespace boost::interprocess;

                mmap_name_ = name;
                create_file(sizeof(header) + capacity * sizeof(row_type));

                file_mapp_ = std::make_unique<boost::interprocess::file_mapping>(mmap_name_.c_str(), read_write);
                region_ = std::make_unique<boost::interprocess::mapped_region>(*file_mapp_, read_write);
                header_ = new (region_->get_address()) header;
                row_ = (row_type *)((char *)region_->get_address() + sizeof(header));

                header_->size = 0;
                header_->capacity = capacity;
                header_->ref_cout = 1;
                header_->lock.clear();
                this->capacity_ = get_region_capacity();
            }

            table(const std::string &name)
            {
                using namespace boost::interprocess;
                mmap_name_ = name;
                file_mapp_ = std::make_unique<boost::interprocess::file_mapping>(mmap_name_.c_str(), read_write);
                region_ = std::make_unique<boost::interprocess::mapped_region>(*file_mapp_, read_write);
                header_ = static_cast<header *>(region_->get_address());
                row_ = (row_type *)((char *)region_->get_address() + sizeof(header));
                header_->ref_cout.fetch_add(1);
                this->capacity_ = get_region_capacity();
            }

            ~table()
            {
                header_->ref_cout.fetch_sub(1);
            }

            size_t push(const value_type &val)
            {
                auto index = header_->size.fetch_add(1);
                return do_push(val, index);
            }

            row_type &operator[](size_t index)
            {
                return do_read(index);
            }

            const row_type &operator[](size_t index) const
            {
                return const_cast<table *>(this)->do_read(index);
            }

            size_t get_size() const
            {
                return header_->size;
            }

            size_t get_capacity() const
            {
                return header_->capacity;
            }

            size_t get_ref_cout() const
            {
                return header_->ref_cout;
            }
        };
    }
}