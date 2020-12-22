#pragma once

#include <stdint.h>

#include <chrono>
#include <string>
#include <sstream>
#include <iomanip>

namespace mio
{
    namespace chrono
    {
        inline std::chrono::nanoseconds now()
        {
            using namespace std::chrono;
            return duration_cast<nanoseconds>(system_clock::now().time_since_epoch());
        }
    } // namespace chrono
} // namespace mio

namespace std
{
    inline std::string to_string(std::chrono::nanoseconds tm)
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
} // namespace std
