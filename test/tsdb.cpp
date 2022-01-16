#include <iostream>
#include <thread>
#include <cstddef>
#include <vector>

#include <gtest/gtest.h>
#include <mio/tsdb.hpp>

constexpr size_t COUNT = 1000000;

constexpr size_t BUFFER_SIZE = 4096;

constexpr size_t THREAD_WRITE_NUM = 1;

constexpr size_t THREAD_READ_NUM = 1;

class verify
{
public:
    template <size_t DATA_SIZE>
    struct value
    {
        size_t val;
        char _[DATA_SIZE - sizeof(val)];
    };

    template <size_t DATA_SIZE>
    void run_one()
    {
        using value = value<DATA_SIZE>;
        std::vector<size_t> array(COUNT);
        std::atomic<bool> flag = false;

        std::chrono::nanoseconds write_diff;
        std::chrono::nanoseconds read_diff;

        std::thread write_thread[THREAD_WRITE_NUM];
        std::thread read_thread[THREAD_READ_NUM];

        for (size_t i = 0; i < THREAD_WRITE_NUM; i++)
        {
            write_thread[i] = std::thread([&]()
                                          {
                mio::tsdb::table<value> table("test.db", BUFFER_SIZE);
                flag = true;
                flag.notify_all();

                value data;

                auto start = std::chrono::steady_clock::now();
                for (size_t i = 0; i < COUNT; i++)
                {
                    data.val = i;
                    table.push(data);
                }
                auto end = std::chrono::steady_clock::now();
                write_diff = end - start; });
        }

        for (size_t i = 0; i < THREAD_READ_NUM; i++)
        {
            read_thread[i] = std::thread([&]()
                                         {

                flag.wait(false);

                mio::tsdb::table<value> table("test.db");

                auto start = std::chrono::steady_clock::now();
                for (size_t i = 0; i < COUNT; i++)
                {
                    table[i].wait();
                    size_t index = table[i].value().val;
                    array[index]++;
                }
                auto end = std::chrono::steady_clock::now();
                read_diff = end - start; });
        }

        for (size_t i = 0; i < THREAD_WRITE_NUM; i++)
        {
            write_thread[i].join();
        }

        for (size_t i = 0; i < THREAD_READ_NUM; i++)
        {
            read_thread[i].join();
        }

        size_t max = 0;
        size_t min = 0;
        for (size_t i = 0; i < COUNT; i++)
        {
            if (array[i] != THREAD_WRITE_NUM)
            {
                if (array[i] > THREAD_WRITE_NUM)
                    max++;
                else
                    min++;
            }
        }

        printf("size/%lu byte\t w/%lu ns\t r/%lu ns\n", DATA_SIZE, write_diff.count() / COUNT, read_diff.count() / COUNT);

        ASSERT_TRUE(max == 0);
        ASSERT_TRUE(min == 0);
    }

    template <size_t... DATA_SIZE_>
    void run()
    {
        (run_one<DATA_SIZE_>(), ...);
    }
};

TEST(tsdb, tsdb)
{
    verify v;
    v.run<64, 128, 256, 512, 1024>();
}

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}