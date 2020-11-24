#pragma once

#include "parallelism/pipe.hpp"

namespace mio
{
    namespace parallelism
    {
        template <typename T_, size_t N_ = 4096>
        class channel
        {
        private:
            mio::parallelism::pipe<T_, N_> pipe_[2];

        public:
                channel() = default;

                template <typename InputIt>
                void write(int fd, InputIt first, size_t count, const wait::handler_t &handler = wait::yield)
                {
                    if(fd)
                    {
                        this->pipe_[0].write(first, count, handler);
                    }
                    else
                    {
                        this->pipe_[1].write(first, count, handler);
                    }
                }

                template <typename OutputIt>
                void read(int fd, OutputIt result, size_t count, const wait::handler_t &handler = wait::yield)
                {
                    if(fd)
                    {
                        this->pipe_[1].read(result, count, handler);
                    }
                    else
                    {
                        this->pipe_[0].read(result, count, handler);
                    }
                }
        };
    } // namespace parallelism
} // namespace mio