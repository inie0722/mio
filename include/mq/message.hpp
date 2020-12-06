#pragma once

#include <vector>
#include <string>

#include <boost/uuid/uuid.hpp>

namespace mio
{
    namespace mq
    {
        using buffer_t = std::vector<char>;

        enum class message_type : int8_t
        {
            REQUEST = 0,
            RESPONSE,
            NOTICE
        };

        struct message
        {
            message_type type;
            boost::uuids::uuid uuid;
            std::string name;
            buffer_t data;
        };
    }
}