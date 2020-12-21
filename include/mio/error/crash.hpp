#pragma once

#include <csignal>
#include <functional>

#include <boost/stacktrace.hpp>

namespace error
{
    namespace detail
    {
        inline std::function<void(int signal, const boost::stacktrace::stacktrace &)> handler;
    }

    void set_crash(const std::function<void(int signal, const boost::stacktrace::stacktrace &)> &handler)
    {
        detail::handler = handler;
        auto signal_handler = [](int signal) {
            detail::handler(signal, boost::stacktrace::stacktrace());
        };

        std::signal(SIGTERM, signal_handler);
        std::signal(SIGSEGV, signal_handler);
        std::signal(SIGINT, signal_handler);
        std::signal(SIGILL, signal_handler);
        std::signal(SIGABRT, signal_handler);
        std::signal(SIGFPE, signal_handler);
    }
} // namespace error
