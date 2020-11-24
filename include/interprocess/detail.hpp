#pragma once

#include "parallelism/channel.hpp"
#include <boost/interprocess/managed_shared_memory.hpp>

namespace mio
{
    namespace interprocess
    {
        namespace detail
        {
            struct request
            {
                boost::interprocess::offset_ptr<parallelism::channel<char>> channel_ptr;
            };

        }
    }
}