#include <iostream>
#include <stdint.h>
#include <stdlib.h>

extern "C"
{
#include "context.h"
}

using namespace std;

void *stack_1;
void *stack_2;

void fun(void *arg)
{
    cout << arg << endl;
    cout << "0x2" << endl;
    cout << swap_context(&stack_2, stack_1, (void *)0x3) << endl;
    cout << "0x6" << endl;
    exit(1);
}

int main()
{
    stack_2 = malloc(1000);
    stack_2 = (char *)stack_2 + 1000;

    create_context(&stack_2, fun);

    cout << swap_context(&stack_1, stack_2, (void *)0x1) << endl;
    cout << "0x4" << endl;
    cout << swap_context(&stack_1, stack_2, (void *)0x5) << endl;

    return 0;
}
