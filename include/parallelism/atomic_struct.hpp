#pragma once

#include <string.h>
#include <stdint.h>

#include <atomic>

namespace libcpp
{
    namespace detail
    {
        template <size_t>
        struct atomic_struct_raw;
        template <>
        struct atomic_struct_raw<1>
        {
            using type = uint8_t;
        };
        template <>
        struct atomic_struct_raw<2>
        {
            using type = uint16_t;
        };
        template <>
        struct atomic_struct_raw<3>
        {
            using type = uint32_t;
        };
        template <>
        struct atomic_struct_raw<4>
        {
            using type = uint32_t;
        };
        template <>
        struct atomic_struct_raw<5>
        {
            using type = uint64_t;
        };
        template <>
        struct atomic_struct_raw<6>
        {
            using type = uint64_t;
        };
        template <>
        struct atomic_struct_raw<7>
        {
            using type = uint64_t;
        };
        template <>
        struct atomic_struct_raw<8>
        {
            using type = uint64_t;
        };
    } // namespace detail

    template <typename T>
    class atomic_struct
    {
    private:
        static_assert(sizeof(T) < sizeof(uint64_t), "Struct is larger than 8 bytes");
        using raw = detail::atomic_struct_raw<sizeof(T)>;

        std::atomic<raw> data;

        raw encode(const T &val)
        {
            raw ret;
            memcpy(&ret, &val, sizeof(val));
            return ret;
        }

        T decode(const raw &val)
        {
            T ret;
            memcpy(&ret, &val, sizeof(val));
            return ret;
        }

    public:
        T operator=(const T &val)
        {
            return decode(data = encode(val));
        }

        bool is_lock_free() const
        {
            return data.is_lock_free();
        }

        void store(const T val, std::memory_order mo = std::memory_order_seq_cst)
        {
            data.store(encode(val), mo);
        }

        T load(std::memory_order mo = std::memory_order_seq_cst)
        {
            return decode(data.load(mo));
        }

        operator T()
        {
            return decode(data);
        }

        T exchange(const T val, std::memory_order mo = std::memory_order_seq_cst)
        {
            return decode(data.exchange(encode(val), mo));
        }

        bool compare_exchange_weak(T &expected, T desired, std::memory_order success = std::memory_order_seq_cst, std::memory_order failure = std::memory_order_seq_cst)
        {
            auto exp = encode(expected);
            auto ret = data.compare_exchange_weak(exp, encode(desired), success, failure);

            if (!ret)
            {
                expected = decode(exp);
            }

            return ret;
        }

        bool compare_exchange_strong(T &expected, T desired, std::memory_order success = std::memory_order_seq_cst, std::memory_order failure = std::memory_order_seq_cst)
        {
            auto exp = encode(expected);
            auto ret = data.compare_exchange_strong(exp, encode(desired), success, failure);

            if (!ret)
            {
                expected = decode(exp);
            }

            return ret;
        }
    };
} // namespace libcpp