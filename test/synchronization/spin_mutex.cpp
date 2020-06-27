#include "synchronization/spin_mutex.hpp"

#include <assert.h>

#include <mutex>
#include <chrono>
#include <iostream>
#include <thread>

using namespace std;

#define SIZE 1250000

#define THREAD_NUM 4

//mutex m;
libcpp::spin_mutex m;

size_t count;

int main(void)
{
    thread thrd[THREAD_NUM];

    for (size_t i = 0; i < THREAD_NUM; i++)
    {
        thrd[i] = thread([]() {
            auto start = std::chrono::steady_clock::now();
            for (size_t i = 0; i < SIZE; i++)
            {
                m.lock();
                ++count;
                m.unlock();
            }
            auto end = std::chrono::steady_clock::now();
            std::chrono::duration<double> diff = end - start;
            std::cout << this_thread::get_id() << "time: " << diff.count() << " s\n";
        });
    }

    for (size_t i = 0; i < THREAD_NUM; i++)
    {
        thrd[i].join();
    }
    

    assert(count == SIZE *THREAD_NUM);
    return 0;
}