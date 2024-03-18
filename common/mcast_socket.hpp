#pragma once

#include <vector>
#include <functional>

#include "socket_utils.hpp"
#include "logger.hpp"

namespace Common {
    constexpr size_t MCastBufferSize = 64 * 1024 * 1024;

    struct MCastSocket {
        explicit MCastSocket(Logger &logger) : logger_(logger) {
            outbound_data_.resize(MCastBufferSize);
            inbound_data_.resize(MCastBufferSize);
        }

        ~MCastSocket() {
            close(socket_fd_);
            socket_fd_ = -1;
        }

        int init(const std::string &ip, const std::string &iface, int port, bool is_listening);
        bool join(const std::string &ip);
        void leave(const std::string &ip, int port);
        void send(const void *data, size_t len) noexcept;
        bool sendAndRecv() noexcept;

        MCastSocket() = delete;
        MCastSocket(const MCastSocket&) = delete;
        MCastSocket(const MCastSocket&&) = delete;
        MCastSocket& operator=(const MCastSocket&) = delete;
        MCastSocket& operator=(const MCastSocket&&) = delete;

        int socket_fd_ = -1;

        std::vector<char> outbound_data_;
        size_t next_send_valid_index_ = 0;
        std::vector<char> inbound_data_;
        size_t next_rcv_valid_index_ = 0;

        std::function<void(MCastSocket* s)> recv_callback_ = nullptr;

        std::string time_str_;
        Logger &logger_;
    };
}