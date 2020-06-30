#pragma once

#include <stdint.h>

namespace libcpp
{
    class buffer
    {
    private:
        char *buf;
        uint64_t w_cur;
        uint64_t r_cur;

    public:
        buffer(char *buf, size_t size)
        {
            this->buf = buf;
            this->w_cur = 0;
            this->r_cur = 0;
        }

        template <typename T>
        void write(const T &val)
        {
            memcpy(&buf[w_cur], &val, sizeof(T));
            w_cur += sizeof(T);
        }

        template <typename T>
        void write(const T val[], size_t size)
        {
            memcpy(&buf[w_cur], val, sizeof(T) * size);
            w_cur += sizeof(T) * size;
        }

        template <typename T>
        void read(T &val)
        {
            memcpy(&val, &buf[r_cur], sizeof(T));
            r_cur += sizeof(T);
        }

        template <typename T>
        void read(T val[], size_t size)
        {
            memcpy(val, &buf[r_cur], sizeof(T) * size);
            r_cur += sizeof(T) * size;
        }
    };
} // namespace libcpp