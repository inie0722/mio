#include "mio/parallelism/pipe.hpp"

#include <assert.h>
#include <stdint.h>

#include <iostream>
#include <thread>

constexpr size_t SIZE = 10000;

constexpr size_t BUF_SIZE = 4096;

constexpr size_t THREAD_WRITE_NUM = 1;

constexpr size_t THREAD_READ_NUM = 1;

#include <vector>

class verify
{
public:
    template <size_t DATA_SIZE_>
    void run_one()
    {
        mio::parallelism::pipe<std::array<char, DATA_SIZE_>> pipe(BUF_SIZE);
        size_t array[SIZE] = {0};

        std::chrono::nanoseconds write_diff;
        std::chrono::nanoseconds read_diff;

        std::thread write_thread[THREAD_WRITE_NUM];
        std::thread read_thread[THREAD_READ_NUM];

        for (size_t i = 0; i < THREAD_WRITE_NUM; i++)
        {
            write_thread[i] = std::thread([&]() {
                std::array<char, DATA_SIZE_> data;

                auto start = std::chrono::steady_clock::now();
                for (size_t i = 0; i < SIZE; i++)
                {
                    *(size_t *)&data[DATA_SIZE_ - sizeof(size_t)] = i;
                    pipe.write(&data, 1);
                }
                auto end = std::chrono::steady_clock::now();
                write_diff = end - start;
            });
        }

        for (size_t i = 0; i < THREAD_READ_NUM; i++)
        {
            read_thread[i] = std::thread([&]() {
                std::array<char, DATA_SIZE_> data;

                auto start = std::chrono::steady_clock::now();
                for (size_t i = 0; i < SIZE; i++)
                {
                    pipe.read(&data, 1);
                    size_t index = *(size_t *)&data[DATA_SIZE_ - sizeof(size_t)];
                    array[index]++;
                }
                auto end = std::chrono::steady_clock::now();
                read_diff = end - start;
            });
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
        for (size_t i = 0; i < SIZE; i++)
        {
            if (array[i] != THREAD_WRITE_NUM)
            {
                if (array[i] > THREAD_WRITE_NUM)
                    max++;
                else
                    min++;
            }
        }

        assert(max == 0);
        assert(min == 0);

        printf("size/%lu byte\t w/%lu ns\t r/%lu ns\n", DATA_SIZE_, write_diff.count() / SIZE, read_diff.count() / SIZE);
    }

    template <size_t... DATA_SIZE_>
    void run()
    {
        (run_one<DATA_SIZE_>(), ...);
    }
};

int main(void)
{
    verify v;
    v.run<64, 128, 256, 512, 1024>();
    return 0;
}