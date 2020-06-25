#include <boost/lockfree/queue.hpp>

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

boost::lockfree::queue<int> *queue;

using namespace std;

mutex m;

void Write()
{
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < SIZE; i++)
    {
        while (!queue->push(i))
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
    memset(Array, 0, SIZE * sizeof(*Array));

    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < SIZE; i++)
    {
        int d;
        while (!queue->pop(d))
            ;
        Array[d]++;
    }

    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> diff = end - start;

    m.lock();
    std::cout << this_thread::get_id() << " read time: " << diff.count() << " s\n";
    m.unlock();
}

int main(void)
{
    queue = new boost::lockfree::queue<int>(BUF_SIZE);
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
