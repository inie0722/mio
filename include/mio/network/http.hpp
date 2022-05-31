#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>

namespace mio
{
    namespace network
    {
        class request
        {
        private:
            boost::asio::io_context &io_context_;
            boost::asio::ssl::context ctx_{boost::asio::ssl::context::tlsv12_client};

        public:
            request(boost::asio::io_context &io_context)
                : io_context_(io_context)
            {
                using namespace boost::asio;
                ctx_.set_default_verify_paths();
                ctx_.set_verify_mode(ssl::verify_peer);
            }

            boost::beast::http::response<boost::beast::http::dynamic_body> http(std::string_view host, std::string_view port, const boost::beast::http::request<boost::beast::http::string_body> &req)
            {
                using namespace boost;
                using namespace boost::asio;

                ip::tcp::resolver resolver(io_context_);
                auto results = resolver.resolve(host, port);

                beast::tcp_stream stream(io_context_);
                stream.connect(results);

                beast::http::write(stream, req);

                beast::flat_buffer buffer;
                beast::http::response<beast::http::dynamic_body> res;
                beast::http::read(stream, buffer, res);

                beast::error_code ec;
                stream.socket().shutdown(asio::ip::tcp::socket::shutdown_both, ec);
                return res;
            }

            boost::beast::http::response<boost::beast::http::dynamic_body> https(std::string_view host, std::string_view port, const boost::beast::http::request<boost::beast::http::string_body> &req)
            {
                using namespace boost;
                using namespace boost::asio;

                ip::tcp::resolver resolver(io_context_);
                auto results = resolver.resolve(host, port);
                beast::ssl_stream<beast::tcp_stream> stream(io_context_, ctx_);

                //握手
                if (!SSL_set_tlsext_host_name(stream.native_handle(), host.data()))
                {
                    beast::error_code ec{static_cast<int>(::ERR_get_error()), error::get_ssl_category()};
                    throw beast::system_error{ec};
                }

                beast::get_lowest_layer(stream).connect(results);
                stream.handshake(ssl::stream_base::client);

                beast::http::write(stream, req);

                beast::flat_buffer buffer;
                beast::http::response<beast::http::dynamic_body> res;
                beast::http::read(stream, buffer, res);

                beast::error_code ec;
                stream.shutdown(ec);
                return res;
            }
        };
    }
}