#include <iostream>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

extern "C"
{
#include "context.h"
}

using namespace std;

void *stack_1;
void *stack_2;

void fun(void *arg)
{
    cout << "1 " << endl;
    swap_context(&stack_2, stack_1);
    cout << "3 " << endl;
    exit(1);
}

int main()
{
    stack_2 = malloc(1000);
    stack_2 = (char *)stack_2 + 1000;

    create_context(&stack_2, fun, (void *)0x1234);

    swap_context(&stack_1, stack_2);
    cout << "2 " << endl;
    swap_context(&stack_1, stack_2);

    return 0;
}
