#pragma once

#include <tbb/spin_mutex.h>

namespace libcpp
{
    using spin_mutex = tbb::spin_mutex;
} // namespace libcpp