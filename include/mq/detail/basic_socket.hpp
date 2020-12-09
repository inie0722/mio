#pragma once

#include <string>
#include <stddef.h>
#include <memory>

namespace mq
{
    namespace detail
    {
        class basic_socket
        {
        public:
            virtual void connect(const std::string &address) = 0;

            virtual size_t write(const void *data, size_t size) = 0;

            virtual size_t read(void *data, size_t size) = 0;

            virtual void close() = 0;

            virtual ~basic_socket() = 0;
        };

        class basic_acceptor
        {
        public:
            virtual void bind(const std::string &address) = 0;
            virtual std::unique_ptr<basic_socket> accept() = 0;
            virtual void close() = 0;
            virtual ~basic_acceptor() = 0;
        };
    } // namespace detail
} // namespace mq