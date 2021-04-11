#pragma once

#include <string_view>
#include <climits>
#include <stdint.h>

namespace mio
{
    namespace metaprogram
    {
        template<typename T>
        class hash
        {
        private:
            uint64_t seed;

            constexpr uint64_t hash_combine(uint64_t h, uint64_t k) const
            {
                h += seed;

                uint64_t m = 0xc6a4a7935bd1e995;
                int r = 47;

                k *= m;
                k ^= k >> r;
                k *= m;

                h ^= k;
                h *= m;

                //防止出现0
                h += 0xe6546b64;

                return h;
            }

        public:
            constexpr hash() : seed(0){};
            constexpr hash(uint64_t seed) : seed(seed){};

            constexpr uint64_t operator()(T k) const
            {
                return hash_combine(seed, k);
            }

            constexpr uint64_t operator()(uint64_t k) const
            {
                return hash_combine(seed, k);
            }

            constexpr uint64_t operator()(int64_t k) const
            {
                return hash_combine(seed, k);
            }

            constexpr uint64_t operator()(double k) const
            {
                uint64_t seed_2 = k * 2.71828 * 3.141592653 * ULLONG_MAX;

                return hash_combine(seed, seed_2);
            }

            constexpr uint64_t operator()(const std::string_view &k) const
            {
                uint64_t seed = 0;
                for (size_t i = 0; i < k.size(); i++)
                {
                    seed = hash_combine(seed, k[i]);
                }

                return seed;
            }
        };
    }
}