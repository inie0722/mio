#pragma once

#include <stdint.h>

#include <thread>
#include <chrono>
#include <string>
#include <sstream>
#include <iomanip>
#include <iostream>

#ifdef WIN32
#include <intrin.h>
#pragma intrinsic(__rdtsc)
#endif

namespace tool
{
    class Timer
    {
    private:
        static uint64_t tsc_clock()
        {
            uint32_t val[2];

#ifdef __GNUC__
            asm volatile(
                "rdtscp \n\t"
                : "=a"(val[0]), "=d"(val[1])
                :
                :);

#elif WIN32
            *(uint64_t *)val = __rdtsc();
#endif
            return *(uint64_t *)val;
        }
        inline static uint64_t start_ns;
        inline static uint64_t start_tsc;
        inline static long double cpu_mhz;

    public:
        Timer()
        {
            using namespace std::chrono_literals;
            using namespace std::chrono;

            start_ns = duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count();
            start_tsc = tsc_clock();

            auto start = tsc_clock();
            std::this_thread::sleep_for(1s);
            auto end = tsc_clock();

            cpu_mhz = (long double)(end - start) / 1000000000;
        }

        static std::chrono::nanoseconds now()
        {
            using namespace std::chrono;
            return nanoseconds((uint64_t)(start_ns + (tsc_clock() - start_tsc) / cpu_mhz));
        }

        static std::string to_string(std::chrono::nanoseconds tm)
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
    };
    inline Timer __timer;
} // namespace tool
