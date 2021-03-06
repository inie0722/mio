#pragma once

#include <boost/functional/hash.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/fiber/all.hpp>

#include <memory>
#include <utility>
#include <unordered_set>
#include <unordered_map>
#include <string>
#include <shared_mutex>
#include <variant>
#include <vector>
#include <atomic>
#include <thread>
#include <list>
#include <functional>

#include "mio/mq/detail/round_robin.hpp"
#include "mio/mq/detail/basic_socket.hpp"

#include "mio/mq/message.hpp"
#include "mio/mq/future.hpp"
#include "mio/mq/transfer.hpp"

namespace mio
{
    namespace mq
    {
        class manager
        {
        public:
            using socket_t = detail::basic_socket;
            using acceptor_t = detail::basic_acceptor;
            class session;

        private:
            using session_set_t = std::unordered_set<std::shared_ptr<session>>;
            using session_list_t = std::list<std::shared_ptr<session>>;

        public:
            class session : public std::enable_shared_from_this<session>
            {
            private:
                friend class manager;
                manager *manager_;

                std::unique_ptr<socket_t> socket_;
                boost::fibers::buffered_channel<std::pair<std::shared_ptr<message>, std::shared_ptr<promise>>> write_pipe_;

                boost::fibers::fiber read_fiber_;
                boost::fibers::fiber write_fiber_;

                //请求消息的回复信息
                boost::fibers::mutex uuid_promise__mutex_;
                std::unordered_map<boost::uuids::uuid, std::shared_ptr<promise>, boost::hash<boost::uuids::uuid>> uuid_promise_map_;

                //该会话加入的所以组
                std::shared_mutex group_map_mutex_;
                std::unordered_map<std::string, session_list_t::iterator> group_map_;

                size_t level_ = 0;

                void do_write()
                {
                    try
                    {
                        while (1)
                        {
                            std::pair<std::shared_ptr<message>, std::shared_ptr<promise>> msg;
                            write_pipe_.pop(msg);

                            if (msg.second != nullptr)
                            {
                                uuid_promise__mutex_.lock();
                                uuid_promise_map_[msg.first->uuid] = msg.second;
                                uuid_promise__mutex_.unlock();
                            }

                            uint64_t name_size = msg.first->name.size();
                            uint64_t data_size = msg.first->data.size();

                            socket_->write(&msg.first->type, sizeof(msg.first->type));
                            socket_->write(&msg.first->uuid, sizeof(msg.first->uuid));
                            socket_->write(&name_size, sizeof(name_size));
                            socket_->write(&data_size, sizeof(data_size));
                            socket_->write(&msg.first->name[0], name_size);
                            socket_->write(&msg.first->data[0], data_size);
                        }
                    }
                    catch (const std::exception &e)
                    {
                        std::cerr << e.what() << '\n';
                    }
                }

                void do_read()
                {
                    try
                    {
                        while (1)
                        {
                            auto msg = std::make_shared<message>();
                            uint64_t name_size;
                            uint64_t data_size;

                            socket_->read(&msg->type, sizeof(msg->type));
                            socket_->read(&msg->uuid, sizeof(msg->uuid));
                            socket_->read(&name_size, sizeof(name_size));
                            socket_->read(&data_size, sizeof(data_size));

                            msg->name.resize(name_size);
                            msg->data.resize(data_size);

                            socket_->read(&msg->name[0], name_size);
                            socket_->read(&msg->data[0], data_size);

                            if (msg->type == message_type::RESPONSE)
                            {
                                uuid_promise_map_[msg->uuid]->set_value(msg);
                            }
                            else
                            {
                                auto &handler = manager_->message_handler_[msg->name];
                                if (std::holds_alternative<message_handler_t>(handler.second))
                                {
                                    boost::fibers::fiber(std::get<message_handler_t>(handler.second), message_args{this->shared_from_this(), msg}).detach();
                                }
                                else
                                {
                                    std::get<message_queue_t>(handler.second)->push(message_args(this->shared_from_this(), std::move(msg)));
                                }
                            }
                        }
                    }
                    catch (const std::exception &e)
                    {
                        std::cerr << e.what() << '\n';
                    }
                }

            public:
                session(manager *manager, decltype(socket_) &&socket) : manager_(manager), socket_(std::move(socket)), write_pipe_(4096)
                {
                    read_fiber_ = boost::fibers::fiber(&session::do_read, this);
                    write_fiber_ = boost::fibers::fiber(&session::do_write, this);
                }

                ~session()
                {
                    std::cout << __func__ << std::endl;
                }

                void close()
                {
                    //触发回调
                    //manager_->close_handler_(*this);

                    //关闭读和写 纤程
                    socket_->close();
                    write_pipe_.close();

                    read_fiber_.join();
                    write_fiber_.join();

                    //移除所有定义列表
                    group_map_mutex_.lock();
                    manager_->group_map_mutex_.lock();
                    for (auto &group : group_map_)
                    {
                        manager_->group_map_[group.first].erase(group.second);
                    }
                    manager_->group_map_mutex_.unlock();
                    group_map_mutex_.unlock();

                    manager_->session_set_mutex_.lock();
                    manager_->session_set_.erase(shared_from_this());
                    manager_->session_set_mutex_.unlock();
                }

                future request(std::shared_ptr<message> &msg)
                {
                    auto promise = std::make_shared<mio::mq::promise>(1);
                    write_pipe_.push({msg, promise});
                    return promise->get_future();
                }

                void unicast(const std::shared_ptr<message> &msg)
                {
                    write_pipe_.push({msg, nullptr});
                }

                void response(const std::shared_ptr<message> &msg)
                {
                    write_pipe_.push({msg, nullptr});
                }

                void add_group(const std::string &group_name)
                {
                    group_map_mutex_.lock();
                    manager_->group_map_mutex_.lock();

                    manager_->group_map_[group_name].push_front(shared_from_this());
                    group_map_[group_name] = manager_->group_map_[group_name].begin();

                    manager_->group_map_mutex_.unlock();
                    group_map_mutex_.unlock();
                }

                void remove_group(const std::string &group_name)
                {
                    group_map_mutex_.lock();
                    manager_->group_map_mutex_.lock();

                    manager_->group_map_[group_name].erase(group_map_[group_name]);
                    group_map_.erase(group_name);

                    manager_->group_map_mutex_.unlock();
                    group_map_mutex_.unlock();
                }

                void set_level(size_t level)
                {
                    level_ = level;
                }
            };

        private:
            friend class session;

            //收到消息的处理程序
            using message_args = std::pair<std::weak_ptr<session>, std::shared_ptr<message>>;
            using message_handler_t = std::function<void(message_args)>;
            using message_queue_t = std::shared_ptr<boost::fibers::buffered_channel<message_args>>;

            std::shared_mutex message_handler_mutex_;
            std::unordered_map<std::string, std::pair<size_t, std::variant<message_handler_t, message_queue_t>>> message_handler_;

            //会话set
            boost::fibers::recursive_mutex session_set_mutex_;
            session_set_t session_set_;

            //组列表
            std::shared_mutex group_map_mutex_;
            std::unordered_map<std::string, session_list_t> group_map_;

            //当接受客户端 或 连接上服务器将 触发的回调
            using verification_handler_t = std::function<void(const std::string &address, std::weak_ptr<session>)>;
            verification_handler_t acceptor_handler_;
            verification_handler_t connect_handler_;

            //会话关闭时触发
            using close_handler_t = std::function<void(session &)>;
            close_handler_t close_handler_;

            //管理所有bind地址
            boost::fibers::mutex acceptor_list_mutex_;
            std::list<std::unique_ptr<acceptor_t>> acceptor_list_;

            //asio调度器分配
            std::atomic<size_t> io_context_count_;
            std::vector<std::shared_ptr<boost::asio::io_context>> io_context_;
            std::vector<std::thread> thread_;
            size_t thread_size_;

            auto get_io_context()
            {
                return io_context_[io_context_count_.fetch_add(1) % thread_size_];
            }

        public:
            manager(size_t thread_size)
            {
                thread_size_ = thread_size;
                for (size_t i = 0; i < thread_size; i++)
                {
                    io_context_.push_back(std::make_unique<boost::asio::io_context>());
                    thread_.push_back(std::thread([this, i]() {
                        boost::fibers::use_scheduling_algorithm<boost::fibers::asio::round_robin>(io_context_[i]);

                        while (1)
                        {
                            try
                            {
                                io_context_[i]->run();
                            }
                            catch (const std::exception &e)
                            {
                                std::cerr << e.what() << '\n';
                            }
                        }
                    }));
                }
            }

            ~manager()
            {
                //关闭所有acceptor
                acceptor_list_mutex_.lock();
                for (auto &acceptor : acceptor_list_)
                {
                    acceptor->close();
                }
                acceptor_list_mutex_.unlock();

                //关闭所有会话
                for (auto &session : session_set_)
                {
                    session->close();
                }
            }

            void on_acceptor(const verification_handler_t &handler)
            {
                acceptor_handler_ = handler;
            }

            void on_connect(const verification_handler_t &handler)
            {
                connect_handler_ = handler;
            }

            void on_colse(const close_handler_t handler)
            {
                close_handler_ = handler;
            }

            void registered(const std::string &name, size_t level, const std::variant<message_handler_t, message_queue_t> &handler)
            {
                message_handler_mutex_.lock();
                message_handler_[name] = {level, handler};
                message_handler_mutex_.unlock();
            }

            void logout(const std::string &name, size_t level)
            {
                message_handler_mutex_.lock();
                message_handler_.erase(name);
                message_handler_mutex_.unlock();
            }

            template <typename Protocol>
            void bind(const std::string &address)
            {
                acceptor_list_mutex_.lock();
                acceptor_list_.push_back(std::make_unique<typename transfer<Protocol>::acceptor>(*get_io_context()));
                auto it = --acceptor_list_.end();
                acceptor_list_mutex_.unlock();

                (*it)->bind(address);

                auto io_context = get_io_context();

                io_context->post([&, it, address]() {
                    boost::fibers::fiber([&, it, address]() {
                        while (1)
                        {
                            auto socket = (*it)->accept(*get_io_context());

                            auto session_ptr = std::make_shared<session>(this, std::move(socket));

                            session_set_mutex_.lock();
                            session_set_.insert(session_ptr);
                            session_set_mutex_.unlock();

                            boost::fibers::fiber(acceptor_handler_, address, session_ptr).detach();
                        }
                    }).detach();
                });
            }

            template <typename Protocol>
            void connect(const std::string &address)
            {
                auto io_context = get_io_context();
                io_context->post([&, address]() {
                    boost::fibers::fiber([&, address]() {
                        auto socket = std::make_unique<typename transfer<Protocol>::socket>(*get_io_context());
                        socket->connect(address);

                        auto session_ptr = std::make_shared<session>(this, std::move(socket));

                        session_set_mutex_.lock();
                        session_set_.insert(session_ptr);
                        session_set_mutex_.unlock();

                        boost::fibers::fiber(connect_handler_, address, session_ptr).detach();
                    }).detach();
                });
            }

            void push(const std::string &group_name, message msg)
            {
                auto message_ptr = std::make_shared<message>(msg);

                group_map_mutex_.lock_shared();
                for (auto &it : group_map_[group_name])
                {
                    it->write_pipe_.push({message_ptr, nullptr});
                }
                group_map_mutex_.unlock_shared();
            }
        };
    } // namespace mq
} // namespace mio
