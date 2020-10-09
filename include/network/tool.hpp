#pragma once

#include <stdint.h>
#include <stddef.h>

#include <type_traits>
#include <memory>

namespace network
{
    bool is_little()
    {
        uint16_t a = 1;

        return (*(char *)&a);
    }

    template <typename T>
    T hton(T val)
    {
        T ret;
        if (is_little())
        {
            if constexpr (sizeof(T) == 1)
            ;
            else if constexpr(sizeof(T) == 2)
            {
                ret = (val >> 8) | (val << 8);
            }
            else if constexpr (sizeof(T) == 4)
            {
                ret = (val << 24) | (val >> 24) | ((val >> 8) & 0xff00) | ((val << 8) & 0xff0000);
            }
            else if constexpr (sizeof(T) == 8)
            {
                ret = (val << 56) | ((val << 40) & 0xff000000000000) 
                | ((val << 24) & 0xff0000000000) | ((val << 8) & 0xff00000000) 
                | ((val >> 8) & 0xff000000) | ((val >> 24) & 0xff0000) 
                | ((val >> 40) & 0xff00) | (val >> 56);
            }
        }
        else
        {
            ret = val;
        }
        return ret;
    }

    template <typename T>
    T ntoh(T val)
    {
        T ret;
        if (is_little())
        {
            if constexpr (sizeof(T) == 1)
            ;
            else if constexpr(sizeof(T) == 2)
            {
                ret = (val >> 8) | (val << 8);
            }
            else if constexpr (sizeof(T) == 4)
            {
                ret = (val << 24) | (val >> 24) | ((val >> 8) & 0xff00) | ((val << 8) & 0xff0000);
            }
            else if constexpr (sizeof(T) == 8)
            {
                ret = (val << 56) | ((val << 40) & 0xff000000000000) 
                | ((val << 24) & 0xff0000000000) | ((val << 8) & 0xff00000000) 
                | ((val >> 8) & 0xff000000) | ((val >> 24) & 0xff0000) 
                | ((val >> 40) & 0xff00) | (val >> 56);
            }
        }
        else
        {
            ret = val;
        }
        return ret;
    }

    class Buffer
    {
    private:
        char *buf;
        size_t w_cur;
        size_t r_cur;

        template <typename T>
        void __write(const T &val)
        {
            memcpy(&buf[w_cur], &val, sizeof(T));
            w_cur += sizeof(T);
        }

        template <typename T>
        void __write(const T val[], size_t size)
        {
            memcpy(&buf[w_cur], val, sizeof(T) * size);
            w_cur += sizeof(T) * size;
        }

        template <typename T>
        void __write_integral(const T &val)
        {
            T n = hton(val);
            __write(n);
        }

        template <typename T>
        void __write_integral(const T val[], size_t size)
        {
            std::unique_ptr<T[]> n(new T[size]);
            for (size_t i = 0; i < size; i++)
            {
                n[i] = hton(val[i]);
            }

            __write(n.get(), size);
        }

        template <typename T>
        void __read(T &val)
        {
            memcpy(&val, &buf[r_cur], sizeof(T));
            r_cur += sizeof(T);
        }

        template <typename T>
        void __read(T val[], size_t size)
        {
            memcpy(val, &buf[r_cur], sizeof(T) * size);
            r_cur += sizeof(T) * size;
        }

        template <typename T>
        void __read_integral(T &val)
        {
            __read(val);
            val = ntoh(val);
        }

        template <typename T>
        void __read_integral(T val[], size_t size)
        {
            __read(val, size);

            for (size_t i = 0; i < size; i++)
            {
                val[i] = ntoh(val[i]);
            }
        }

    public:
        Buffer(void *buf)
        {
            this->buf = (char *)buf;
            this->w_cur = 0;
            this->r_cur = 0;
        }

        template <typename T>
        void write(const T &val)
        {
            //如果是整数 就转换字节序 再写入
            if constexpr (std::is_integral<T>::value || std::is_floating_point<T>::value)
            {
                __write_integral(val);
            }
            else
            {
                __write(val);
            }
        }

        template <typename T>
        void write(const T val[], size_t size)
        {
            //如果是整数 就转换字节序 再写入
            if constexpr (std::is_integral<T>::value || std::is_floating_point<T>::value)
            {
                __write_integral(val, size);
            }
            else
            {
                __write(val, size);
            }
        }

        template <typename T>
        void read(T &val)
        {
            //如果是整数 就转换字节序 再写入
            if constexpr (std::is_integral<T>::value || std::is_floating_point<T>::value)
            {
                __read_integral(val);
            }
            else
            {
                __read(val);
            }
        }

        template <typename T>
        void read(T val[], size_t size)
        {
            //如果是整数 就转换字节序 再写入
            if constexpr (std::is_integral<T>::value || std::is_floating_point<T>::value)
            {
                __read_integral(val, size);
            }
            else
            {
                __read(val, size);
            }
        }

        size_t r_size()
        {
            return r_cur;
        }

        size_t w_size()
        {
            return w_cur;
        }
    };
} // namespace network
