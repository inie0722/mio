#include <iostream>
#include <assert.h>
#include <boost/stacktrace.hpp>

#define assertion(expr, msg)                          \
    if (!(expr))                                      \
    {                                                 \
        std::cerr << msg << '\n';                     \
        std::cerr << boost::stacktrace::stacktrace(); \
        assert(expr);                                 \
    }
