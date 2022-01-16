#include <iostream>
#include <thread>
#include <queue>
#include <string>

#include <mio/log.hpp>
#include <gtest/gtest.h>

constexpr size_t SIZE = 99999;

class stream
{
private:
    std::queue<std::string> queue_;

public:
    stream &operator<<(const std::string &str)
    {
        queue_.push(str);
        return *this;
    }

    std::string pop()
    {
        auto ret = queue_.front();
        queue_.pop();
        return ret;
    }
};

TEST(log, log)
{
    mio::log LOG;

    std::thread th([&]()
                   { decltype(LOG)::run(); });

    stream stream_;

    auto start = mio::chrono::now();
    for (size_t i = 0; i < SIZE; i++)
    {
        LOG(stream_, std::string_view("{} {} {}\n"), i, std::to_string(i * 3.14), i * 8.25);
    }
    auto end = mio::chrono::now();

    printf("size/%lu ns/%lu \n", SIZE, (end - start).count() / SIZE);
    while (LOG.size())
        ;

    decltype(LOG)::stop();
    th.join();

    for (size_t i = 0; i < SIZE; i++)
    {
        GTEST_ASSERT_EQ(stream_.pop(), fmt::format("{} {} {}\n", i, std::to_string(i * 3.14), i * 8.25));
    }
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}