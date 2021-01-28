#pragma once

#include "interprocess/shared_memory.hpp"
#include "parallelism/pipe.hpp"
#include "serialization/binary.hpp"
#include "type_traits.hpp"
#include "chrono.hpp"

#include <fmt/format.h>
#include <fmt/args.h>

#include <boost/interprocess/containers/list.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>
#include <mutex>
#include <atomic>
#include <fstream>
#include <stdint.h>

#include <iostream>

namespace mio
{
    namespace detail
    {
        class log_base
        {
        protected:
            enum class log_type : uint8_t
            {
                STATIC = 1,
                DYNAMIC = 2,
                OPEN = 3,
                CLOSE = 4
            };

            struct log_line
            {
                log_type type;
                uint64_t size;
                char data[];
            };

            struct log_arg
            {
                struct open
                {
                    uint64_t file_id;
                    char file_name[];
                };

                struct close
                {
                    uint64_t file_id;
                };

                struct static_data
                {
                    uint64_t line_id;
                    char format[];
                };

                struct dynamic_data
                {
                    uint64_t file_id;
                    uint64_t line_id;
                    uint64_t data_size;
                    char data[];
                };
            };
        };
    } // namespace detail

    template <size_t BUUFER_SIZE_ = 4096>
    class log_client : public detail::log_base
    {
    private:
        using pipe_t = parallelism::pipe<char, mio::interprocess::allocator<char>>;
        using list_pipe_t = boost::interprocess::list<boost::interprocess::offset_ptr<pipe_t>, interprocess::allocator<interprocess::offset_ptr<pipe_t>>>;

        std::string shm_name_;
        std::unique_ptr<interprocess::managed_mapped_file> shared_memory_;
        list_pipe_t *list_pipe_;
        typename list_pipe_t::iterator pipe_it_;

        boost::interprocess::interprocess_mutex *mutex_;

        std::atomic<uint64_t> *file_id;

        std::atomic<uint64_t> *line_id;

        std::unique_ptr<char[]> buffer;

        void constructor()
        {
            using namespace boost::interprocess;

            thread_local bool flag = true;
            if (flag == true)
            {
                buffer = std::unique_ptr<char[]>(new char[BUUFER_SIZE_]);
                shared_memory_ = std::make_unique<interprocess::managed_mapped_file>(open_only, shm_name_.c_str());

                list_pipe_ = shared_memory_->find<list_pipe_t>("list_pipe").first;
                mutex_ = shared_memory_->find<boost::interprocess::interprocess_mutex>("mutex").first;
                file_id = shared_memory_->find<std::atomic<uint64_t>>("file_id").first;
                line_id = shared_memory_->find<std::atomic<uint64_t>>("line_id").first;

                mutex_->lock();
                list_pipe_->push_front(shared_memory_->construct<pipe_t>(anonymous_instance)(65536, pipe_t::allocator_type(shared_memory_->get_segment_manager())));
                pipe_it_ = list_pipe_->begin();
                mutex_->unlock();

                flag = false;
            }
        }

    public:
        log_client(const std::string &shm_name) : shm_name_(shm_name)
        {
        }
        ~log_client()
        {
            while ((**pipe_it_).size())
            {
                std::this_thread::yield();
            }

            mutex_->lock();
            shared_memory_->destroy_ptr((*pipe_it_).get());
            list_pipe_->erase(pipe_it_);
            mutex_->unlock();
        }

        uint64_t open_file(const std::string &file)
        {
            constructor();
            //打开文件操作
            log_line *line = (log_line *)buffer.get();
            line->type = log_type::OPEN;
            line->size = sizeof(log_arg::open) + file.length() + 1;

            log_arg::open *arg = (log_arg::open *)line->data;
            memcpy(arg->file_name, file.c_str(), file.length() + 1);
            arg->file_id = file_id->fetch_add(1);

            (**pipe_it_).write(reinterpret_cast<char *>(line), sizeof(*line) + line->size);
            return arg->file_id;
        }

        void close_file(uint64_t file_id)
        {
            constructor();
            //关闭文件操作
            log_line *line = (log_line *)buffer.get();
            line->type = log_type::CLOSE;
            line->size = sizeof(log_arg::close);

            log_arg::close *arg = (log_arg::close *)line->data;
            arg->file_id = file_id;

            (**pipe_it_).write(reinterpret_cast<char *>(line), sizeof(*line) + line->size);
        }

        uint64_t send_static(const std::string &format)
        {
            constructor();
            //发送静态数据
            log_line *line = (log_line *)buffer.get();
            line->type = log_type::STATIC;
            log_arg::static_data *static_data = (log_arg::static_data *)line->data;
            static_data->line_id = line_id->fetch_add(1);

            memcpy(static_data->format, format.c_str(), format.length() + 1);

            line->size = sizeof(*static_data) + format.length() + 1;
            (**pipe_it_).write(reinterpret_cast<char *>(line), sizeof(*line) + line->size);
            return static_data->line_id;
        }

        template <typename... Args_>
        void send_dynamic(uint64_t file_id, uint64_t line_id, const Args_ &... args)
        {
            constructor();
            //发送动态数据
            log_line *line = (log_line *)buffer.get();
            line->type = log_type::DYNAMIC;

            log_arg::dynamic_data *dynamic_data = (log_arg::dynamic_data *)line->data;
            dynamic_data->file_id = file_id;
            dynamic_data->line_id = line_id;

            serialization::binary binary(dynamic_data->data);

            binary.pack(args...);

            dynamic_data->data_size = binary.get_write_size();
            line->size = sizeof(*dynamic_data) + dynamic_data->data_size;

            (**pipe_it_).write(reinterpret_cast<char *>(line), sizeof(*line) + line->size);
        }
    };

    template <size_t PIPE_SIZE_ = 65536, size_t BUUFER_SIZE_ = 4096>
    class log_service : public detail::log_base
    {
    private:
        using pipe_t = parallelism::pipe<char, mio::interprocess::allocator<char>>;
        using list_pipe_t = boost::interprocess::list<boost::interprocess::offset_ptr<pipe_t>, mio::interprocess::allocator<interprocess::offset_ptr<pipe_t>>>;

        std::string shm_name_;

        std::unique_ptr<interprocess::managed_mapped_file> shared_memory_;
        list_pipe_t *list_pipe_;
        typename list_pipe_t::iterator pipe_it_;

        boost::interprocess::interprocess_mutex *mutex_;

        std::atomic<uint64_t> *file_id;

        std::atomic<uint64_t> *line_id;

        std::unique_ptr<char[]> buffer;

        fmt::dynamic_format_arg_store<fmt::format_context> fmt_args_;

        std::unordered_map<uint64_t, std::fstream> file_map_;
        std::unordered_map<uint64_t, std::string> fmt_map_;

        void open(const log_arg::open &arg)
        {
            file_map_[arg.file_id].open(arg.file_name, std::ios::app);
        }

        void close(const log_arg::close &arg)
        {
            file_map_.erase(arg.file_id);
        }

        void static_data(const log_arg::static_data &arg)
        {
            fmt_map_[arg.line_id] = arg.format;
        }

        void dynamic_data(log_arg::dynamic_data &arg)
        {
            serialization::binary buf(arg.data);

            auto push = [&](auto tmp) {
                buf >> tmp;
                fmt_args_.push_back(tmp);
            };

            while (buf.get_read_size() < arg.data_size)
            {
                switch (buf.get_type_id())
                {
                case type_id_v<int8_t>:
                    push(int8_t());
                    break;
                case type_id_v<uint8_t>:
                    push(uint8_t());
                    break;
                case type_id_v<int16_t>:
                    push(int16_t());
                    break;
                case type_id_v<uint16_t>:
                    push(uint16_t());
                    break;
                case type_id_v<int32_t>:
                    push(int32_t());
                    break;
                case type_id_v<uint32_t>:
                    push(uint32_t());
                    break;
                case type_id_v<int64_t>:
                    push(int64_t());
                    break;
                case type_id_v<uint64_t>:
                    push(uint64_t());
                    break;
                case type_id_v<float>:
                    push(float());
                    break;
                case type_id_v<double>:
                    push(double());
                    break;
                case type_id_v<long double>:
                {
                    long double tmp;
                    buf >> tmp;
                    fmt_args_.push_back(tmp);
                    break;
                }
                case type_id_v<std::string>:
                    push(std::string());
                    break;
                case type_id<std::chrono::nanoseconds>::value:
                {
                    std::chrono::nanoseconds tmp;
                    buf >> tmp;
                    fmt_args_.push_back(mio::to_string(tmp));
                    break;
                }
                default:
                    break;
                }
            }

            file_map_[arg.file_id] << fmt::vformat(fmt_map_[arg.line_id], fmt_args_);
            file_map_[arg.file_id].flush();
            fmt_args_.clear();
        }

    public:
        log_service(const std::string &shm_name, size_t shm_size)
        {
            using namespace boost::interprocess;
            boost::interprocess::shared_memory_object::remove(shm_name.c_str());
            shared_memory_ = std::make_unique<mio::interprocess::managed_mapped_file>(boost::interprocess::create_only, shm_name.c_str(), shm_size);

            list_pipe_ = shared_memory_->construct<list_pipe_t>("list_pipe")(typename list_pipe_t::allocator_type(shared_memory_->get_segment_manager()));
            mutex_ = shared_memory_->construct<boost::interprocess::interprocess_mutex>("mutex")();
            file_id = shared_memory_->construct<std::atomic<uint64_t>>("file_id")();
            line_id = shared_memory_->construct<std::atomic<uint64_t>>("line_id")();
        }

        void run(std::chrono::milliseconds ms)
        {
            auto buffer = std::unique_ptr<char[]>(new char[BUUFER_SIZE_]);
            log_line *line = (log_line *)buffer.get();

            while (1)
            {
                mutex_->lock();

                for (auto &it : *list_pipe_)
                {
                    if ((*it).size() > sizeof(*line))
                    {
                        (*it).read(buffer.get(), sizeof(*line));
                        (*it).read(line->data, line->size);
                        switch (line->type)
                        {
                        case log_type::OPEN:
                            open(*(log_arg::open *)line->data);
                            break;
                        case log_type::STATIC:
                            static_data(*(log_arg::static_data *)line->data);
                            break;
                        case log_type::DYNAMIC:
                            dynamic_data(*(log_arg::dynamic_data *)line->data);
                            break;
                        case log_type::CLOSE:
                            close(*(log_arg::close *)line->data);
                            break;
                        }
                    }
                }

                mutex_->unlock();

                std::this_thread::sleep_for(ms);
            }
        }
    };
} // namespace mio