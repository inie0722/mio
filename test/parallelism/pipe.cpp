#include "parallelism/pipe.hpp"

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include <iostream>
#include <atomic>
#include <thread>
#include <mutex>

using namespace std;

#define SIZE 1250000 * 8 * 2

#define BUF_SIZE 2048

#define THREAD_WRITE_NUM 1

#define THREAD_READ_NUM 1

libcpp::pipe<> *queue;

using namespace std;

mutex m;

struct test
{
    char t[3500];
    int data;
};

void Write()
{
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < SIZE; i++)
    {
        test t;
        t.data = i;
        //queue->send(&t, sizeof(t));
        
        while(!queue->try_send(&t, sizeof(t)))
        ;
        
    }

    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = end - start;

    m.lock();
    std::cout << this_thread::get_id() << " write time: " << diff.count() << " s\n";
    m.unlock();
}

std::atomic<int> *Array = new std::atomic<int>[SIZE];

void Read()
{
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < SIZE; i++)
    {
        test d;
//        queue->recv(&d, sizeof(d));

        while(!queue->try_recv(&d, sizeof(d)))
        ;

        Array[d.data]++;
    }

    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = end - start;

    m.lock();
    std::cout << this_thread::get_id() << " read time: " << diff.count() << " s\n";
    m.unlock();
}

#include <numeric>
#include <chrono>

int main(void)
{
    memset(Array, 0, SIZE * sizeof(*Array));
    queue = new libcpp::pipe<>;

    std::thread read_thread[THREAD_READ_NUM];
    for (size_t i = 0; i < THREAD_READ_NUM; i++)
    {
        read_thread[i] = thread(Read);
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

    int max = 0;
    int min = 0;
    for (int i = 0; i < SIZE; i++)
    {
        if (Array[i] != THREAD_WRITE_NUM)
        {
            if (Array[i] > THREAD_WRITE_NUM)
                max++;
            else
                min++;
        }
    }

    cout << max << endl;
    cout << min << endl;

    assert(max == 0);
    assert(min == 0);
    delete[] Array;
    return 0;
}