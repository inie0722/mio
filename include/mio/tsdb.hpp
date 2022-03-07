/**
 * @file tsdb.hpp
 * @author 然Y (inie0722@gmail.com)
 * @brief
 * @version 0.1
 * @date 2022-03-07
 *
 * @copyright Copyright (c) 2022
 *
 */
#pragma once

#include <atomic>
#include <cstdint>
#include <fstream>
#include <memory>
#include <filesystem>

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>

namespace mio
{
    /**
     * @brief 时间序列 数据库
     *
     */
    namespace tsdb
    {
        /**
         * @brief 行
         * @details 数据库 访问最小单位
         * @tparam T 存储的类型
         * @tparam Atomic atomic类型 默认采用std 如果需要进程间使用，则需要改为 boost::ipc_atomic
         */
        template <typename T, template <typename> typename Atomic = std::atomic>
        class row
        {
        public:
            using value_type = T; /// 存储类型

        private:
            Atomic<bool> is_write_;
            value_type value_;

        public:
            /**
             * @brief 对内容赋值
             *
             * @param val
             * @return row&
             */
            row &operator=(const value_type &val)
            {
                value_ = val;
                is_write_ = true;
                is_write_.notify_all();
                return *this;
            }

            /**
             * @brief 访问所含值
             *
             * @return value_type&
             */
            value_type &operator*()
            {
                return value_;
            }

            /**
             * @brief 访问所含值
             *
             * @return const value_type&
             */
            const value_type &operator*() const
            {
                return value_;
            }

            /**
             * @brief 访问所含值
             *
             * @return value_type*
             */
            value_type *operator->()
            {
                return &value_;
            }

            /**
             * @brief 访问所含值
             *
             * @return const value_type*
             */
            const value_type *operator->() const
            {
                return &value_;
            }

            /**
             * @brief 检查 *this 是否含值
             *
             * @return true *this 含值
             * @return false *this 不含值
             */
            bool has_value() const
            {
                return is_write_;
            }

            /**
             * @brief 检查 *this 是否含值
             *
             * @return true *this 含值
             * @return false *this 不含值
             */
            operator bool() const
            {
                return has_value();
            }

            /**
             * @brief 阻塞等待
             *
             */
            void wait() const
            {
                is_write_.wait(false);
            }

            /**
             * @brief 返回所含值
             *
             * @return value_type&
             */
            value_type &value()
            {
                if (!is_write_)
                {
                    throw std::runtime_error("bad row access");
                }

                return value_;
            }

            /**
             * @brief 返回所含值
             *
             * @return value_type&
             */
            const value_type &value() const
            {
                return const_cast<row *>(this)->value();
            }

            /**
             * @brief 若所含值可用则返回它，否则返回另一个值
             *
             * @tparam U U&& 必须可转换为 value_type
             * @param default_value 用于 *this 为空情况的值
             * @return value_type
             */
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

        /**
         * @brief 表
         * @details 每个表包括N个行
         * @tparam T 存储类型
         * @tparam Atomic atomic类型 默认才用std 如果需要进程间使用，则需要改为 boost::ipc_atomic
         */
        template <typename T, template <typename> typename Atomic = std::atomic>
        class table
        {
        public:
            using value_type = T;
            using row_type = row<value_type, Atomic>;

        private:
            struct header
            {
                Atomic<std::uint64_t> size;
                Atomic<std::uint64_t> capacity;
                Atomic<std::uint64_t> ref_cout;
                Atomic<bool> lock;
            };

            std::string mmap_name_;
            std::unique_ptr<boost::interprocess::file_mapping> file_mapp_;
            std::unique_ptr<boost::interprocess::mapped_region> region_;

            header *header_;
            row_type *row_;

            /// 本地 capacity
            std::uint64_t capacity_;

            void create_file(size_t size)
            {
                std::filebuf fbuf;
                fbuf.open(mmap_name_, std::ios::in | std::ios::out | std::ios::trunc | std::ios::binary);
                fbuf.pubseekoff(size - 1, std::ios::beg);
                fbuf.sputc(0);
            }

            /// 重新设置文件大小
            void recapacity()
            {
                std::filesystem::resize_file(mmap_name_, sizeof(header) + header_->capacity * 2 * sizeof(row_type));
                header_->capacity = header_->capacity * 2;
            }

            /// 重新映射
            void remmap()
            {
                using namespace boost::interprocess;

                region_->~mapped_region();
                new (region_.get()) mapped_region(*file_mapp_, read_write);
                header_ = static_cast<header *>(region_->get_address());

                row_ = reinterpret_cast<row_type *>(header_ + 1);
                capacity_ = this->get_region_capacity();
            }

            /// 获取现在映射的内存的 capacity
            size_t get_region_capacity() const
            {
                return (region_->get_size() - sizeof(header)) / sizeof(row_type);
            }

            /// 推入数据
            size_t do_push(const value_type &val, size_t index)
            {
                if (index >= capacity_)
                {
                    auto flag = header_->lock.exchange(true);

                    if (!flag)
                    {
                        this->recapacity();
                        header_->lock = false;
                        header_->capacity.notify_all();
                    }

                    header_->capacity.wait(capacity_);
                    this->remmap();

                    return this->do_push(val, index);
                }

                row_[index] = val;

                return index;
            }

            /// 读取数据
            row_type &do_read(size_t index)
            {
                if (index >= capacity_)
                {
                    auto flag = header_->lock.exchange(true);
                    if (!flag)
                    {
                        this->recapacity();
                        header_->lock = false;
                        header_->capacity.notify_all();
                    }

                    header_->capacity.wait(capacity_);

                    this->remmap();

                    return this->do_read(index);
                }
                return row_[index];
            }

        public:
            /**
             * @brief 创建一个 table
             *
             * @param name 文件名
             * @param capacity 初始缓存大小
             */
            table(const std::string &name, size_t capacity)
                : mmap_name_(name)
            {
                using namespace boost::interprocess;

                this->create_file(sizeof(header) + capacity * sizeof(row_type));

                file_mapp_ = std::make_unique<file_mapping>(mmap_name_.c_str(), read_write);
                region_ = std::make_unique<mapped_region>(*file_mapp_, read_write);
                header_ = new (region_->get_address()) header;
                row_ = reinterpret_cast<row_type *>(header_ + 1);

                header_->size = 0;
                header_->capacity = capacity;
                header_->ref_cout = 1;
                header_->lock = false;
                capacity_ = this->get_region_capacity();
            }

            /**
             * @brief 打开一个已存在的 table
             *
             * @param name 文件名
             */
            table(const std::string &name)
                : mmap_name_(name)
            {
                using namespace boost::interprocess;

                file_mapp_ = std::make_unique<file_mapping>(mmap_name_.c_str(), read_write);
                region_ = std::make_unique<mapped_region>(*file_mapp_, read_write);
                header_ = static_cast<header *>(region_->get_address());
                row_ = reinterpret_cast<row_type *>(header_ + 1);
                header_->ref_cout.fetch_add(1);
                capacity_ = this->get_region_capacity();
            }

            /**
             * @brief 析构 table
             *
             */
            ~table()
            {
                header_->ref_cout.fetch_sub(1);
            }

            /**
             * @brief 在最后一行写入数据
             *
             * @param val
             * @return size_t 表的总大小
             */
            size_t push(const value_type &val)
            {
                auto index = header_->size.fetch_add(1);
                return this->do_push(val, index);
            }

            /**
             * @brief 访问行
             *
             * @param index
             * @return row_type&
             */
            row_type &operator[](size_t index)
            {
                return this->do_read(index);
            }

            /**
             * @brief 访问行
             *
             * @param index
             * @return const row_type&
             */
            const row_type &operator[](size_t index) const
            {
                return const_cast<table *>(this)->do_read(index);
            }

            /**
             * @brief 返回行数
             *
             * @return size_t
             */
            size_t size() const
            {
                return header_->size;
            }

            /**
             * @brief 返回当前存储空间能够容纳的行数
             *
             * @return size_t
             */
            size_t capacity() const
            {
                return header_->capacity;
            }

            /**
             * @brief 返回当前table打开次数
             *
             * @return size_t
             */
            size_t ref_cout() const
            {
                return header_->ref_cout;
            }

            /**
             * @brief 紧缩table 使capacity等于size
             *
             */
            void shrink_to_fit()
            {
                std::filesystem::resize_file(mmap_name_, sizeof(header) + header_->size * sizeof(row_type));
                header_->capacity = header_->size.load();
            }
        };
    }
}