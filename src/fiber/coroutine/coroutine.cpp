#include "coroutine.hpp"

extern "C"
{
#include "context.h"
}

#include <string.h>

using namespace std;
using namespace libcpp;

void coroutine::save_context()
{
    size_t size = this->run_stack.end - this->run_stack.top;
    this->alone_stack.size = size;

    if (this->alone_stack.capacity < size)
    {
        this->alone_stack.base = unique_ptr<char[]>(new char[size * 2]);
        this->alone_stack.capacity = size * 2;
    }

    memcpy(this->alone_stack.base.get(), this->run_stack.top, size);
}

coroutine::coroutine(void (*const start)(void *arg), const shared_ptr<char[]> &stack, const size_t stack_size)
{
    //初始化运行栈
    this->run_stack.begin = stack;
    this->run_stack.size = stack_size;
    this->run_stack.end = this->run_stack.begin.get() + stack_size;
    this->run_stack.top = this->run_stack.end;

    create_context(&this->run_stack.top, start);

    //初始化 独立栈
    this->alone_stack.size = this->run_stack.end - this->run_stack.top;
    this->alone_stack.capacity = this->alone_stack.size;
    this->alone_stack.base = unique_ptr<char[]>(new char[this->alone_stack.capacity]);

    memcpy(this->alone_stack.base.get(), this->run_stack.top, this->alone_stack.size);
}

void *coroutine::resume(void *arg)
{
    memcpy(this->run_stack.top, this->alone_stack.base.get(), this->alone_stack.size);

    void *ret = swap_context(&caller_context, this->run_stack.top, arg);
    this->save_context();
    return ret;
}

void *coroutine::yeid(void *ret)
{
    return swap_context(&this->run_stack.top, this->caller_context, ret);
}

coroutine::~coroutine()
{
}
