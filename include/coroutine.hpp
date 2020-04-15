#pragma once

#include <stddef.h>

#include <memory>

class coroutine
{

private:
    struct
    {
        std::shared_ptr<char[]> begin;
        char *end;
        char *top;
        size_t size;
    } run_stack;

    struct
    {
        std::unique_ptr<char[]> base;
        size_t size;
        size_t capacity;
    } alone_stack;

    char *caller_context;

    void save_context();

public:
    coroutine(void (*const start)(void *arg), const std::shared_ptr<char[]> &stack, const size_t stack_size);

    void *resume(void *arg);

    void *yeid(void *ret);

    ~coroutine();
};
