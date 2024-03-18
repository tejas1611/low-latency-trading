#pragma once

#include <vector>

#include "socket_utils.hpp"

namespace Common {
    constexpr size_t McastBufferSize = 64 * 1024 * 1024;

    struct MCastSocket {
        explicit MCastSocket(Logger &logger) : logger_(logger) {
            outbound_data_.resize(McastBufferSize);
            inbound_data_.resize(McastBufferSize);
        }

        ~MCastSocket() {
            close(socket_fd_);
            socket_fd_ = -1;
        }

        auto init(const std::string &ip, const std::string &iface, int port, bool is_listening) -> int;
        auto join(const std::string &ip) -> bool;
        auto leave(const std::string &ip, int port) -> void;
        auto send(const void *data, size_t len) noexcept -> void;
        auto sendAndRecv() noexcept -> bool;

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
    };
}