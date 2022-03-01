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
            std::vector<std::string> strs;
            boost::split(strs, time, boost::is_any_of("."));

            tm tm_;
            strptime(strs[0].c_str(), "%Y-%m-%d %H:%M:%S", &tm_);
            return std::chrono::nanoseconds(std::chrono::seconds(mktime(&tm_))) + std::chrono::nanoseconds(std::stoull(strs[1], nullptr, 0));
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

        std::chrono::nanoseconds ns = rtime % 1s;

        std::stringstream ss;
        std::chrono::time_point<std::chrono::system_clock> tt(std::chrono::duration_cast<std::chrono::milliseconds>(rtime));
        auto t = std::chrono::system_clock::to_time_t(tt);
        ss << std::put_time(std::localtime(&t), "%F %T.");

        ss << ns.count();
        return ss.str();
    }
} // namespace mio
