#pragma once

#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stddef.h>

#include <iostream>
#include <fstream>
#include <thread>
#include <atomic>
#include <list>
#include <mutex>
#include <sstream>
#include <iomanip>

#include "parallelism/pipe.hpp"

namespace libcpp
{
    template <typename Format, size_t PIPE_SIZE = 65536, size_t BUUFER_SIZE = 4096>
    class log
    {
    private:
        inline static std::list<pipe<PIPE_SIZE>> list;
        inline static thread_local typename std::list<pipe<PIPE_SIZE>>::iterator thread_pipe;
        inline static thread_local char buffer[BUUFER_SIZE];
        inline static thread_local size_t length = 0;

        inline static std::mutex mutex;

        std::string time_to_string(std::chrono::nanoseconds tm)
        {

            using namespace std::chrono;
            using namespace std::chrono_literals;

            std::chrono::time_point<std::chrono::system_clock> tt(std::chrono::duration_cast<std::chrono::milliseconds>(tm));

            unsigned long long all_ns = tm.count();
            unsigned long long ms = (unsigned long long)(all_ns % 31536000000000000 % 86400000000000 % 3600000000000 % 60000000000 % 1000000000) / 1000000;
            unsigned long long us = (unsigned long long)(all_ns % 31536000000000000 % 86400000000000 % 3600000000000 % 60000000000 % 1000000000 % 1000000) / 1000;
            unsigned long long ns = (unsigned long long)all_ns % 31536000000000000 % 86400000000000 % 3600000000000 % 60000000000 % 1000000000 % 1000000 % 1000;

            std::stringstream ss;
            auto t = std::chrono::system_clock::to_time_t(tt);
            ss << std::put_time(std::localtime(&t), "%F %T ");

            char buf[128];
            sprintf(buf, "%04llu:%04llu:%04llu", ms, us, ns);

            ss << buf;
            return ss.str();
        }

    public:
        static log &get_log()
        {
            static thread_local log ret;
            return ret;
        }

        log()
        {
            mutex.lock();
            list.push_front(pipe<PIPE_SIZE>());
            thread_pipe = list.begin();
            mutex.unlock();
        }

        ~log()
        {
            mutex.lock();
            list.erase(thread_pipe);
            mutex.unlock();
        }

        void run(std::chrono::milliseconds ms)
        {
            //64m大小的 共享内存
            while (1)
            {
                mutex.lock();
                for (auto it = list.begin(); it != list.end(); ++it)
                {
                    if (it->try_recv(&length, sizeof(size_t)))
                    {
                        it->recv(buffer, length);
                        unpack();
                    }
                }
                mutex.unlock();

                std::this_thread::sleep_for(ms);
            }
        }

        template <typename... Args>
        void operator()(const char *file, const char *format, Args... args)
        {
            length = sizeof(length);
            pack(file, format, args...);
        }

    private:
#define LOG_PACK(Type, Val)                 \
    template <typename... Args>             \
    void pack(Type first, Args... args)     \
    {                                       \
        buffer[length++] = Val;             \
        *((Type *)&buffer[length]) = first; \
        length += sizeof(first);            \
        pack(args...);                      \
    }

        template <typename... Args>
        void pack()
        {
            length -= sizeof(length);
            *((size_t *)&buffer[0]) = length;
            thread_pipe->send(buffer, length + sizeof(length));
        }

        LOG_PACK(int8_t, 1)
        LOG_PACK(uint8_t, 2)
        LOG_PACK(int16_t, 3)
        LOG_PACK(uint16_t, 4)
        LOG_PACK(int32_t, 5)
        LOG_PACK(uint32_t, 6)
        LOG_PACK(int64_t, 7)
        LOG_PACK(uint64_t, 8)
        LOG_PACK(float, 9)
        LOG_PACK(double, 10)
        LOG_PACK(long double, 11)

        template <typename... Args>
        void pack(const char *str, Args... args)
        {
            buffer[length++] = 12;
            size_t str_len = strlen(str) + 1;
            memcpy(&buffer[length], str, str_len);
            length += str_len;

            pack(args...);
        }

        LOG_PACK(std::chrono::nanoseconds, 13)

        void unpack()
        {
            //文件名
            size_t len = 1;
            std::ofstream file(&buffer[len], std::ios_base::app);

            len = strlen(&buffer[len]) + 2;

            len++;
            Format format(&buffer[len]);
            len += strlen(&buffer[len]) + 1;

            for (size_t i = len; i < length;)
            {

                switch (buffer[i++])
                {
                case 1:
                    format % *(int8_t *)&buffer[i];
                    i += sizeof(int8_t);
                    break;
                case 2:
                    format % *(uint8_t *)&buffer[i];
                    i += sizeof(uint8_t);
                    break;
                case 3:
                    format % *(int16_t *)&buffer[i];
                    i += sizeof(int16_t);
                    break;
                case 4:
                    format % *(uint16_t *)&buffer[i];
                    i += sizeof(uint16_t);
                    break;

                case 5:
                    format % *(int32_t *)&buffer[i];
                    i += sizeof(int32_t);
                    break;
                case 6:
                    format % *(uint32_t *)&buffer[i];
                    i += sizeof(uint32_t);
                    break;
                case 7:
                    format % *(int64_t *)&buffer[i];
                    i += sizeof(int64_t);
                    break;
                case 8:
                    format % *(uint64_t *)&buffer[i];
                    i += sizeof(uint64_t);
                    break;
                case 9:
                    format % *(float *)&buffer[i];
                    i += sizeof(float);
                    break;
                case 10:
                    format % *(double *)&buffer[i];
                    i += sizeof(double);
                    break;
                case 11:
                    format % *(long double *)&buffer[i];
                    i += sizeof(long double);
                    break;

                case 12:
                    format % &buffer[i];
                    i += strlen(&buffer[i]) + 1;
                    break;

                case 13:
                    format % time_to_string(*(std::chrono::nanoseconds *)&buffer[i]);
                    i += sizeof(std::chrono::nanoseconds);
                    break;

                default:
                    break;
                }
            }

            file << format;
        }
    };

} // namespace libcpp