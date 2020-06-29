#include<iostream>

#include "network/network.hpp"

using namespace libcpp;
using namespace std;

void test(void *socket , void *arg, uint32_t arg_size, void *ret, uint32_t *ret_size)
{
    cout << 123 << endl;
    char msg[] = "123456789.0";
    memcpy(ret, msg, sizeof(msg));
    *ret_size = sizeof(msg);
}

void push()
{
    cout << 321 << endl;
}

int main(void)
{
    network::service s(20);
    s.bind("test", test);
    s.registered("test");

    s.run("0.0.0.0", 9999);
    char msg[] = "123456789.0";

    while (1)
    {
        s.push("test", msg, sizeof(msg));
    }
    
    return 0;
}