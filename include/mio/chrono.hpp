#pragma once

#include <stdint.h>

#include <chrono>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/date_time.hpp>

namespace mio
{
    namespace chrono
    {
        inline std::chrono::nanoseconds now()
        {
            using namespace std::chrono;
            return duration_cast<nanoseconds>(system_clock::now().time_since_epoch());
        }

        inline std::chrono::nanoseconds to_time(const std::string &time)
        {
            using namespace std::chrono_literals;
            std::vector<std::string> strs[4];
            boost::split(strs[0], time, boost::is_any_of(" "));

            boost::split(strs[1], strs[0][0], boost::is_any_of("-"));
            boost::posix_time::ptime end{boost::gregorian::date(atoll(strs[1].at(0).c_str()), atoll(strs[1].at(1).c_str()), atoll(strs[1].at(2).c_str()))};
            boost::posix_time::time_duration diff = end - boost::posix_time::ptime(boost::gregorian::date(1970, 1, 1));
            auto ret = std::chrono::nanoseconds(diff.total_nanoseconds()) - 8h;

            if (strs[0].size() >= 2)
            {
                boost::split(strs[2], strs[0][1], boost::is_any_of(":"));
                ret += std::stoull(strs[2][0]) * 1h;
                ret += std::stoull(strs[2][1]) * 1min;
                ret += std::stoull(strs[2][2]) * 1s;
            }

            if (strs[0].size() >= 3)
            {
                boost::split(strs[3], strs[0][2], boost::is_any_of(":"));
                ret += std::stoull(strs[3][0]) * 1ms;
                ret += std::stoull(strs[3][1]) * 1us;
                ret += std::stoull(strs[3][2]) * 1ns;
            }

            return ret;
        }

        class stopwatch
        {
        private:
            std::chrono::nanoseconds start_ = mio::chrono::now();

        public:
            std::chrono::nanoseconds operator()() const
            {
                return mio::chrono::now() - this->start_;
            }
        };
    } // namespace chrono

    template <typename Rep, typename Period>
    inline std::string to_string(const std::chrono::duration<Rep, Period> &rtime)
    {

        using namespace std::chrono;
        using namespace std::chrono_literals;

        unsigned long long ms = (unsigned long long)(rtime % 1s / 1ms);
        unsigned long long us = (unsigned long long)(rtime % 1ms / 1us);
        unsigned long long ns = (unsigned long long)(rtime % 1us / 1ns);

        std::stringstream ss;
        std::chrono::time_point<std::chrono::system_clock> tt(std::chrono::duration_cast<std::chrono::milliseconds>(rtime));
        auto t = std::chrono::system_clock::to_time_t(tt);
        ss << std::put_time(std::localtime(&t), "%F %T ");

        char buf[128];
        sprintf(buf, "%03llu:%03llu:%03llu", ms, us, ns);

        ss << buf;
        return ss.str();
    }
} // namespace mio
