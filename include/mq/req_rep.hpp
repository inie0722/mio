#pragma once

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <boost/unordered_map.hpp>

#include <list>
#include <mutex>
#include <memory>

#include "parallelism/ring_queue.hpp"

#include "network/tcp.hpp"

namespace mio
{
    namespace mq
    {
        namespace req_rep
        {
            struct request
            {
                boost::uuids::uuid uuid;
                std::vector<char> data;
            };

            struct response
            {
                boost::uuids::uuid uuid;
                std::vector<char> data;
            };

            class rep
            {
            private:
                using response_queue_t = parallelism::ring_queue<response>;
                using request_queue_t = parallelism::ring_queue<std::pair<response_queue_t *, request>>;

                using socket_t = network::tcp::socket;
                using acceptor_t = network::tcp::acceptor;
                using socket_list_t = std::list<socket_t>;

                request_queue_t request_queue_;
                socket_list_t socket_list_;
                std::mutex mutex_;

                boost::asio::io_context &io_context_;
                acceptor_t acceptor_;

                class session;
                friend class session;

                class session : public std::enable_shared_from_this<session>
                {
                private:
                    rep *rep_;
                    socket_list_t::iterator socket_;
                    response_queue_t response_queue_;

                    void do_read()
                    {
                        boost::asio::spawn(rep_->io_context_, [&](boost::asio::yield_context yield) {
                            auto shared_this_ptr = this->shared_from_this();

                            try
                            {
                                while (1)
                                {
                                    request req;
                                    uint64_t size;

                                    socket_->read(&req.uuid, sizeof(req.uuid), yield);
                                    socket_->read(&size, sizeof(size), yield);
                                    req.data.resize(size);
                                    socket_->read(&req.data[0], size, yield);

                                    rep_->request_queue_.push({&response_queue_, req}, [&](size_t) {
                                        if (!rep_->is_open_)
                                            throw std::runtime_error("Session closed");
                                        rep_->io_context_.post(yield);
                                    });
                                }
                            }
                            catch (...)
                            {
                                socket_->close();
                                std::throw_with_nested(std::runtime_error(std::string(__FILE__) + " " + __func__ + " " + std::to_string(__LINE__)));
                            }
                        });
                    }

                    void do_write()
                    {
                        boost::asio::spawn(rep_->io_context_, [&](boost::asio::yield_context yield) {
                            auto shared_this_ptr = this->shared_from_this();
                            try
                            {
                                while (1)
                                {
                                    response res;
                                    response_queue_.pop(res, [&](size_t) {
                                        if (!rep_->is_open_)
                                            throw std::runtime_error("Session closed");
                                        rep_->io_context_.post(yield);
                                    });

                                    uint64_t size = res.data.size();

                                    socket_->write(&res.uuid, sizeof(res.uuid), yield);
                                    socket_->write(&size, sizeof(size), yield);
                                    socket_->write(&res.data[0], size, yield);
                                }
                            }
                            catch (...)
                            {
                                socket_->close();
                                std::throw_with_nested(std::runtime_error(std::string(__FILE__) + " " + __func__ + " " + std::to_string(__LINE__)));
                            }
                        });
                    }

                public:
                    session(rep *rep, socket_t &&socket)
                    {
                        rep_ = rep;
                        ++rep_->socket_count_;

                        std::lock_guard(rep_->mutex_);
                        rep_->socket_list_.push_back(std::move(socket));
                        socket_ = --rep_->socket_list_.end();
                    }

                    ~session()
                    {
                        --rep_->socket_count_;

                        std::lock_guard(rep_->mutex_);
                        rep_->socket_list_.erase(socket_);
                    }

                    void run()
                    {
                        do_read();
                        do_write();
                    }
                };

                std::atomic<uint64_t> socket_count_;

                std::atomic<bool> is_open_ = true;

            public:
                rep(decltype(io_context_) &io_context) : io_context_(io_context), acceptor_(io_context_)
                {
                }

                void bind(const std::string &address)
                {
                    acceptor_.bind(address);
                    boost::asio::spawn(io_context_, [&](boost::asio::yield_context yield) {
                        try
                        {
                            socket_count_ = 1;
                            while (1)
                            {
                                socket_t socket(io_context_);
                                acceptor_.accept(socket, yield);

                                auto sessio = std::make_shared<session>(this, std::move(socket));
                                sessio->run();
                            }
                        }
                        catch (...)
                        {
                            --socket_count_;
                            std::throw_with_nested(std::runtime_error(std::string(__FILE__) + " " + __func__ + " " + std::to_string(__LINE__)));
                        }
                    });
                }

                void *read(request &req)
                {
                    std::pair<response_queue_t *, request> pair;
                    request_queue_.pop(pair);

                    req = std::move(pair.second);
                    return pair.first;
                }

                void write(void *channel, response &&res)
                {
                    static_cast<response_queue_t *>(channel)->push(std::move(res));
                }

                void close()
                {
                    is_open_ = false;
                    acceptor_.close();

                    for (auto &val : socket_list_)
                    {
                        val.close();
                    }

                    while (socket_count_)
                    {
                    }
                }
            };

            class future;

            class promise
            {
            private:
                friend class future;
                std::atomic<bool> flag = false;
                response res_;

            public:
                promise() = default;
                ~promise() = default;

                void set_value(decltype(res_) &&res)
                {
                    res_.uuid = res.uuid;
                    res_.data = std::move(res.data);
                    flag = true;
                }
            };

            class future
            {
            private:
                std::shared_ptr<promise> promise_;
                boost::asio::io_context &io_context_;

            public:
                future(const decltype(promise_) &promise, decltype(io_context_) &io_context) : promise_(promise), io_context_(io_context)
                {
                }

                response get()
                {
                    while (!promise_->flag)
                    {
                        std::this_thread::yield();
                    }

                    return promise_->res_;
                }

                response get(boost::asio::yield_context yield)
                {
                    while (!promise_->flag)
                    {
                        io_context_.post(yield);
                    }

                    return promise_->res_;
                }

                ~future() = default;
            };

            class req
            {
            private:
                using socket_t = network::tcp::socket;
                using request_queue_t = parallelism::ring_queue<std::pair<std::shared_ptr<promise>, request>>;

                boost::asio::io_context &io_context_;
                socket_t socket_;
                boost::unordered_map<boost::uuids::uuid, std::shared_ptr<promise>> promise_map_;
                request_queue_t request_queue_;
                std::mutex mutex_;

                std::atomic<bool> is_open_;

                void do_write()
                {
                    boost::asio::spawn(io_context_, [&](boost::asio::yield_context yield) {
                        try
                        {
                            while (1)
                            {
                                std::pair<std::shared_ptr<promise>, request> req;
                                request_queue_.pop(req, [&](size_t) {
                                    if (!is_open_)
                                        throw std::runtime_error("Session closed");
                                    io_context_.post(yield);
                                });

                                {
                                    std::lock_guard lock(mutex_);
                                    promise_map_[req.second.uuid] = req.first;
                                }

                                uint64_t size = req.second.data.size();

                                socket_.write(&req.second.uuid, sizeof(req.second.uuid), yield);
                                socket_.write(&size, sizeof(size), yield);
                                socket_.write(&req.second.data[0], size, yield);
                            }
                        }
                        catch (...)
                        {
                            std::throw_with_nested(std::runtime_error(std::string(__FILE__) + " " + __func__ + " " + std::to_string(__LINE__)));
                        }
                    });
                }

                void do_read()
                {
                    boost::asio::spawn(io_context_, [&](boost::asio::yield_context yield) {
                        try
                        {
                            while (1)
                            {
                                response res;

                                uint64_t size;
                                socket_.read(&res.uuid, sizeof(res.uuid), yield);
                                socket_.read(&size, sizeof(size), yield);
                                res.data.resize(size);
                                socket_.read(&res.data[0], size, yield);

                                {
                                    std::lock_guard lock(mutex_);
                                    promise_map_.at(res.uuid)->set_value(std::move(res));
                                }
                            }
                        }
                        catch (...)
                        {
                            std::throw_with_nested(std::runtime_error(std::string(__FILE__) + " " + __func__ + " " + std::to_string(__LINE__)));
                        }
                    });
                }

            public:
                req(decltype(io_context_) io_context) : io_context_(io_context), socket_(io_context_)
                {
                }

                void connect(const std::string &address)
                {
                    is_open_ = true;
                    socket_.connect(address);
                    do_read();
                    do_write();
                }

                future write(request &&req)
                {
                    auto promise_ptr = std::make_shared<promise>();
                    future ret(promise_ptr, io_context_);
                    request_queue_.push({promise_ptr, req});
                    return ret;
                }

                void close()
                {
                    socket_.close();
                }
            };

        } // namespace req_rep
    }     // namespace mq
} // namespace mio