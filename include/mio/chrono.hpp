/**
 * @file chrono.hpp
 * @author 然Y (inie0722@gmail.com)
 * @brief
 * @version 0.1
 * @date 2022-03-04
 *
 * @copyright Copyright (c) 2022
 *
 */
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
    /**
     * @brief 时间模块
     *
     */
    namespace chrono
    {
        /**
         * @brief 获取当前时间
         *
         * @return std::chrono::nanoseconds
         */
        inline std::chrono::nanoseconds now()
        {
            using namespace std::chrono;
            return duration_cast<nanoseconds>(system_clock::now().time_since_epoch());
        }

        /**
         * @brief 将字符串 转换为时间
         *
         * @param time 格式 2000-08-21 00:07:22.0
         * @return std::chrono::nanoseconds
         */
        inline std::chrono::nanoseconds to_time(const std::string &time)
        {
            using namespace std::chrono_literals;
            std::vector<std::string> strs;
            boost::split(strs, time, boost::is_any_of("."));

            tm tm_;
            strptime(strs[0].c_str(), "%Y-%m-%d %H:%M:%S", &tm_);
            return std::chrono::nanoseconds(std::chrono::seconds(mktime(&tm_))) + std::chrono::nanoseconds(std::stoull(strs[1]));
        }

        /**
         * @brief 计时器
         *
         */
        class stopwatch
        {
        private:
            std::chrono::nanoseconds start_ = mio::chrono::now();

        public:
            /**
             * @brief 获取 计算器构造至调用时经过的纳秒数
             *
             * @return std::chrono::nanoseconds
             */
            std::chrono::nanoseconds operator()() const
            {
                return mio::chrono::now() - this->start_;
            }
        };
    } // namespace chrono

    /**
     * @brief 将时间转换为字符串
     *
     * @tparam Rep 表示计次数的算术类型
     * @tparam Period 表示计次周期
     * @param rtime
     * @return std::string 格式 2000-08-21 00:07:22.0
     */
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
