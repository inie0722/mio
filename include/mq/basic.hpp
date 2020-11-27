#pragma once

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>

namespace mio
{
    namespace mq
    {
        template <typename Protocol>
        class basic_session
        {
        public:
            typedef typename Protocol::socket_t socket_t;
            typedef typename Protocol::acceptor_t acceptor_t;

            virtual void run(boost::asio::yield_context yield) = 0;

            virtual ~basic_session() = default;
        };

        template <typename Protocol>
        class basic_service
        {
        public:
            typedef typename Protocol::socket_t socket_t;
            typedef typename Protocol::acceptor_t acceptor_t;
            typedef basic_session<Protocol> basic_session_t;

            virtual std::shared_ptr<basic_session_t> make_session(const std::shared_ptr<socket_t> &socket) = 0;

            virtual ~basic_service() = default;
        };
    } // namespace mq
} // namespace mio