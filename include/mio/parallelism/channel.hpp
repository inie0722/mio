#pragma once

#include <stdint.h>

#include <atomic>

#include "mio/parallelism/pipe.hpp"

namespace mio
{
    namespace parallelism
    {
        template <typename T_, size_t N_ = 4096>
        class channel
        {
        public:
            static constexpr uint8_t CONNECTED = 0;
            static constexpr uint8_t DISCONNECTED = 1;

        private:
            mio::parallelism::pipe<T_, N_> pipe_[2];
            std::atomic<uint8_t> status_ = CONNECTED;

        public:
            channel() = default;

            template <typename InputIt>
            size_t write_some(bool fd, InputIt first, size_t count, const wait::handler_t &handler = wait::yield)
            {
                auto callback = [&](size_t i) {
                    if (this->status_ == DISCONNECTED)
                        throw std::runtime_error("The peer is disconnected");
                    handler(i);
                };
                return this->pipe_[fd ? 0 : 1].write_some(first, count, callback);
            }

            template <typename OutputIt>
            size_t read_some(bool fd, OutputIt result, size_t count, const wait::handler_t &handler = wait::yield)
            {
                auto callback = [&](size_t i) {
                    if (this->status_ == DISCONNECTED)
                        throw std::runtime_error("The peer is disconnected");
                    handler(i);
                };
                return this->pipe_[fd ? 1 : 0].read_some(result, count, callback);
            }

            template <typename InputIt>
            size_t write(bool fd, InputIt first, size_t count, const wait::handler_t &handler = wait::yield)
            {
                auto callback = [&](size_t i) {
                    if (this->status_ == DISCONNECTED)
                        throw std::runtime_error("The peer is disconnected");
                    handler(i);
                };

                return this->pipe_[fd ? 0 : 1].write(first, count, callback);
            }

            template <typename OutputIt>
            size_t read(bool fd, OutputIt result, size_t count, const wait::handler_t &handler = wait::yield)
            {
                auto callback = [&](size_t i) {
                    if (this->status_ == DISCONNECTED)
                        throw std::runtime_error("The peer is disconnected");
                    handler(i);
                };

                return this->pipe_[fd ? 1 : 0].read(result, count, callback);
            }

            void close()
            {
                status_ = DISCONNECTED;
            }

            uint8_t get_status()
            {
                return status_;
            }
        };
    } // namespace parallelism
} // namespace mio