#include "parallelism/disruptor.hpp"

#include <string.h>

#include <iostream>
#include <thread>
#include <mutex>
#include <numeric>
#include <chrono>

#define SIZE 1250000

#define BUF_SIZE 1024

#define THREAD_WRITE_NUM 8

#define THREAD_READ_NUM 8

libcpp::disruptor<int, THREAD_READ_NUM, BUF_SIZE> *queue;

using namespace std;

mutex m;

void Write()
{

    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < SIZE; i++)
    {
        //queue->push(i);
        
        while(!queue->try_push(i))
            ;

    }

    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = end - start;

    m.lock();
    std::cout << this_thread::get_id() << " write time: " << diff.count() << " s\n";
    m.unlock();
}

void Read(libcpp::disruptor<int, THREAD_READ_NUM, BUF_SIZE>::reader *r)
{
    int *array = new int[SIZE];
    memset(array, 0, SIZE * sizeof(*array));

    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < SIZE * THREAD_WRITE_NUM; i++)
    {
        int d;
        //r->pull(d);
        
        while (!r->try_pull(d))
            ;
        
        array[d]++;
    }

    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = end - start;

    m.lock();
    std::cout << this_thread::get_id() << " read time: " << diff.count() << " s\n";
    m.unlock();

    int max = 0;
    int min = 0;
    for (int i = 0; i < SIZE; i++)
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
    delete[] array;
}

int main(void)
{
    queue = new libcpp::disruptor<int, THREAD_READ_NUM, BUF_SIZE>;
    libcpp::disruptor<int, THREAD_READ_NUM, BUF_SIZE>::reader *r[THREAD_READ_NUM];
    std::thread read_thread[THREAD_READ_NUM];
    for (size_t i = 0; i < THREAD_READ_NUM; i++)
    {
        r[i] = queue->make_reader();
        read_thread[i] = thread(Read, r[i]);
    }

    std::thread write_thread[THREAD_WRITE_NUM];
    for (size_t i = 0; i < THREAD_WRITE_NUM; i++)
    {
        write_thread[i] = thread(Write);
    }

    for (size_t i = 0; i < THREAD_WRITE_NUM; i++)
    {
        write_thread[i].join();
    }

    for (size_t i = 0; i < THREAD_READ_NUM; i++)
    {
        read_thread[i].join();
    }

    return 0;
}
