#pragma once

#include "parallelism/pipe.hpp"

namespace mio
{
    namespace parallelism
    {
        template <size_t SIZE_ = 4096>
        class channel
        {
        public:
            class pipe
            {
            private:
                friend channel;
                channel *channel_;
                size_t send_index_;
                size_t recv_index_;

                pipe(channel *channel, bool flag) : channel_(channel)
                {
                    if (flag == true)
                    {
                        this->send_index_ = 0;
                        this->recv_index_ = 1;
                    }
                    else
                    {
                        this->send_index_ = 1;
                        this->recv_index_ = 0;
                    }
                };

            public:
                pipe(const pipe &other)
                {
                    this->channel_ = other.channel_;
                    this->send_index_ = other.send_index_;
                    this->recv_index_ = other.recv_index_;
                }

                void send(const void *data, size_t size)
                {
                    this->channel_->pipe_[send_index_].send(data, size);
                }

                void recv(void *data, size_t size)
                {
                    this->channel_->pipe_[recv_index_].recv(data, size);
                }
            };

        private:
            friend pipe;
            mio::parallelism::pipe<SIZE_> pipe_[2];

        public:
            pipe make_pipe(bool flag)
            {
                return pipe(this, flag);
            }
        };
    } // namespace parallelism
} // namespace mio