#pragma once

#include "mq/basic.hpp"
#include "mq/msg.hpp"

#include <functional>
#include <unordered_map>

namespace mio
{
    namespace mq
    {
        namespace rpc
        {
            template <typename Protocol>
            class service : public basic_service<Protocol>
            {
            public:
                typedef basic_service<Protocol> basic_service_t;
                typedef basic_session<Protocol> basic_session_t;

                typedef typename Protocol::socket_t socket_t;
                typedef typename Protocol::acceptor_t acceptor_t;

            private:
                typedef std::function<bool(void *socket, const msg_t &arg, msg_t &ret)> Handler;
                std::unordered_map<std::string, Handler> handler_;

            public:
                service() = default;

                class session : public std::enable_shared_from_this<session>, public basic_session_t
                {
                private:
                    friend class service;
                    std::unordered_map<std::string, Handler> &handler_;
                    std::shared_ptr<socket_t> socket_;

                    session(const decltype(socket_) &socket, decltype(handler_) &handler) : handler_(handler), socket_(socket)
                    {
                    }

                public:
                    ~session() = default;

                    void run(boost::asio::yield_context yield)
                    {
                        using namespace boost::asio;

                        msg_t arg_msg;
                        msg_t ret_msg;

                        uint64_t arg_size;
                        uint64_t ret_size;

                        std::string name;
                        uint64_t name_size;

                        while (1)
                        {
                            //读取参数
                            socket_->async_read(&name_size, sizeof(name_size), yield);
                            name.resize(name_size);
                            socket_->async_read(&name[0], name_size, yield);

                            socket_->async_read(&arg_size, sizeof(arg_size), yield);
                            arg_msg.resize(arg_size);

                            //调用回调函数
                            bool flag = handler_.at(name)(socket_.get(), arg_msg, ret_msg);
                            socket_->async_write(&flag, 1, yield);
                            if (!flag)
                            {
                                return;
                            }

                            //发送返回参数
                            ret_size = ret_msg.size();
                            socket_->async_write(&ret_size, sizeof(ret_size), yield);
                            socket_->async_write(&ret_msg[0], ret_size, yield);
                        }
                    }
                };

                std::shared_ptr<basic_session_t> make_session(const std::shared_ptr<socket_t> &socket)
                {
                    return std::shared_ptr<basic_session_t>(new session(socket, handler_));
                }

                //绑定远程调用函数
                void bind(const std::string &name, const Handler &handler)
                {
                    this->handler_[name] = handler;
                }
            };

            template <typename Protocol>
            class client : public std::enable_shared_from_this<client<Protocol>>
            {
            private:
                typedef typename Protocol::socket_t socket_t;
                typedef typename Protocol::acceptor_t acceptor_t;

                typedef std::vector<char> msg_t;

                std::shared_ptr<socket_t> socket_;

            public:
                client(const std::shared_ptr<socket_t> &socket) : socket_(socket)
                {
                }

                ~client()
                {
                    socket_->close();
                }

                msg_t call(const std::string &name, const msg_t &arg)
                {
                    //发送参数
                    size_t name_size = name.length() + 1;
                    socket_->write(&name_size, sizeof(name_size));
                    socket_->write(name.c_str(), name_size);

                    size_t arg_size = arg.size();
                    socket_->write(&arg_size, sizeof(arg_size));
                    socket_->write(&arg[0], arg_size);

                    //读取 标志位
                    bool flag;
                    socket_->read(&flag, sizeof(flag));
                    if (!flag)
                    {
                        throw std::runtime_error("rpc call failed");
                    }

                    msg_t ret;
                    uint64_t ret_size;
                    socket_->read(&ret_size, sizeof(ret_size));
                    ret.resize(ret_size);
                    socket_->read(ret[0], ret_size);

                    return ret;
                }
            };
        } // namespace rpc
    }     // namespace mq
} // namespace mio
