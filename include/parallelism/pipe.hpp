#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <atomic>
#include <thread>

namespace libcpp
{
    template <size_t SIZE = 4096>
    class pipe
    {
    private:
        alignas(64) char buffer[SIZE];

        alignas(64) std::atomic<uint64_t> readable_limit = 0;
        alignas(64) std::atomic<uint64_t> writable_limit = 0;

        size_t get_index(const size_t index) const
        {
            return index & (SIZE - 1);
        }

    public:
        pipe()
        {
        }

        pipe(pipe && src)
        {
        }

        void send(const void *data, size_t size)
        {
            //等待可写
            while (readable_limit + SIZE < writable_limit + size)
                std::this_thread::yield();

            if ((writable_limit % SIZE) + size > SIZE)
            {
                auto len = SIZE - (writable_limit % SIZE);
                memcpy(&this->buffer[get_index(writable_limit)], data, len);
                memcpy(&this->buffer[get_index(writable_limit + len)], (char *)data + len, size - len);
            }
            else
                memcpy(&this->buffer[get_index(writable_limit)], data, size);
            writable_limit += size;
        }

        bool try_send(const void *data, size_t size)
        {
            //等待可写
            if (readable_limit + SIZE < writable_limit + size)
                return false;

            if ((writable_limit % SIZE) + size > SIZE)
            {
                auto len = SIZE - (writable_limit % SIZE);
                memcpy(&this->buffer[get_index(writable_limit)], data, len);
                memcpy(&this->buffer[get_index(writable_limit + len)], (char *)data + len, size - len);
            }
            else
                memcpy(&this->buffer[get_index(writable_limit)], data, size);
            writable_limit += size;
            return true;
        }

        void recv(void *data, size_t size)
        {
            //等待可读
            while (readable_limit + size > writable_limit)
                std::this_thread::yield();

            if ((readable_limit % SIZE) + size > SIZE)
            {
                auto len = SIZE - (readable_limit % SIZE);
                memcpy(data, &this->buffer[get_index(readable_limit)], len);
                memcpy((char *)data + len, &this->buffer[get_index(readable_limit + len)], size - len);
            }
            else
                memcpy(data, &this->buffer[get_index(readable_limit)], size);
            readable_limit += size;
        }

        bool try_recv(void *data, size_t size)
        {
            //等待可读
            if (readable_limit + size > writable_limit)
                return false;

            if ((readable_limit % SIZE) + size > SIZE)
            {
                auto len = SIZE - (readable_limit % SIZE);
                memcpy(data, &this->buffer[get_index(readable_limit)], len);
                memcpy((char *)data + len, &this->buffer[get_index(readable_limit + len)], size - len);
            }
            else
                memcpy(data, &this->buffer[get_index(readable_limit)], size);
            readable_limit += size;
            return true;
        }
    };
} // namespace libcpp