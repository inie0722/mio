#include <iostream>

using namespace std;
#include "coroutine.hpp"

void test(void *arg)
{
    auto co = (coroutine *)arg;
    cout << "0x1" << endl;

    cout << co->yeid((void *)0x2) << endl;
    cout << "0x5" << endl;

    co->yeid((void *)0x6);
}

#include <thread>

int main(void)
{
    shared_ptr<char[]> buf(new char[1000 * 1000]);

    coroutine co(test, buf, 1000 * 1000);
    cout << co.resume(&co) << endl;
    cout << "0x3" << endl;

    cout << co.resume((void *)0x4) << endl;
    return 0;
}
