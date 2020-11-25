#pragma once

#include "parallelism/channel.hpp"
#include <boost/interprocess/managed_shared_memory.hpp>

namespace mio
{
    namespace interprocess
    {
        namespace pipe
        {
            namespace detail
            {
                struct request
                {
                    boost::interprocess::offset_ptr<parallelism::channel<char>> channel_ptr;
                };

            } // namespace detail
        }     // namespace pipe
    }         // namespace ipc
} // namespace mio