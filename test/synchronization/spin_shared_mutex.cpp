#include "synchronization/spin_shared_mutex.hpp"

#include <assert.h>

#include <thread>
#include <chrono>
#include <iostream>
#include <shared_mutex>

#define SIZE 1000000

#define THREAD_NUM 2

using namespace std;

int main(void)
{
    libcpp::spin_shared_mutex mutex;
    bool flag = 0;
    char array[2] = {1, 1};
    thread write[THREAD_NUM];
    thread read[THREAD_NUM];
    for (size_t i = 0; i < THREAD_NUM; i++)
    {
        write[i] = thread([&]() {
            auto start = chrono::steady_clock::now();
            for (size_t i = 0; i < SIZE; i++)
            {
                mutex.lock();
                array[flag] = 0;
                flag = !flag;
                array[flag] = 1;
                mutex.unlock();
            }
            auto end = chrono::steady_clock::now();
            chrono::duration<double> diff = end - start;
            cout << this_thread::get_id() << " write time: " << diff.count() << " s\n";
        });

        read[i] = thread([&]() {
            auto start = chrono::steady_clock::now();
            for (size_t i = 0; i < SIZE; i++)
            {
                mutex.lock_shared();
                assert(array[flag] == 1);
                mutex.unlock_shared();
            }
            auto end = chrono::steady_clock::now();
            chrono::duration<double> diff = end - start;
            cout << this_thread::get_id() << " read time: " << diff.count() << " s\n";
        });
    }

    for (size_t i = 0; i < THREAD_NUM; i++)
    {
        write[i].join();
        read[i].join();
    }

    return 0;
}