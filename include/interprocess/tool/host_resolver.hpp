#pragma once

#include <string>
#include <vector>

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/algorithm/string.hpp>

namespace mio
{
    namespace interprocess
    {
        auto host_resolve(boost::asio::io_context &io_context, const std::string &protocol, const std::string &host, const std::string &port)
        {
            boost::asio::ip::tcp::resolver resolver(io_context);

            boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), host, port);

            if (protocol == "ipv6")
            {
                query = boost::asio::ip::tcp::resolver::query(boost::asio::ip::tcp::v6(), host, port);
            }

            boost::system::error_code ec;
            boost::asio::ip::tcp::resolver::results_type results = resolver.resolve(query, ec);

            if (ec)
                throw std::runtime_error("address resolution failed");

            return results;
        }

        auto host_resolve(boost::asio::io_context &io_context, const std::string &address)
        {
            std::vector<std::string> vec;
            boost::split(vec, address, boost::is_any_of(":"));
            return host_resolve(io_context, vec[0], vec[1], vec[2]);
        }

        auto async_host_resolve(boost::asio::io_context &io_context, const std::string &protocol, const std::string &host, const std::string &port, boost::asio::yield_context yield)
        {
            boost::asio::ip::tcp::resolver resolver(io_context);

            boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), host, port);

            if (protocol == "ipv6")
            {
                query = boost::asio::ip::tcp::resolver::query(boost::asio::ip::tcp::v6(), host, port);
            }

            boost::system::error_code ec;
            boost::asio::ip::tcp::resolver::results_type results = resolver.async_resolve(query, yield[ec]);

            if (ec)
                throw std::runtime_error("address resolution failed");

            return results;
        }

        auto async_host_resolve(boost::asio::io_context &io_context, const std::string &address, boost::asio::yield_context yield)
        {
            std::vector<std::string> vec;
            boost::split(vec, address, boost::is_any_of(":"));
            return async_host_resolve(io_context, vec[0], vec[1], vec[2], yield);
        }
    } // namespace interprocess
} // namespace mio