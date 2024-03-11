#pragma once

#include <functional>

#include "socket_utils.hpp"
#include "logger.hpp"

namespace Common {
    constexpr size_t TCPBufferSize = 64 * 1024 * 1024;

    struct TCPSocket {
        explicit TCPSocket(Logger &logger) : logger_(logger) {
            outbound_data_.resize(TCPBufferSize);
            inbound_data_.resize(TCPBufferSize);
        }

        ~TCPSocket() {
            close(socket_fd_);
            socket_fd_ = -1;
        }

        auto connect(const std::string &ip, const std::string &iface, int port, bool is_listening) -> int;
        auto sendAndRecv() noexcept -> bool;
        auto send(const void *data, size_t len) noexcept -> void;

        TCPSocket() = delete;
        TCPSocket(const TCPSocket &) = delete;
        TCPSocket(const TCPSocket &&) = delete;
        TCPSocket &operator=(const TCPSocket &) = delete;
        TCPSocket &operator=(const TCPSocket &&) = delete;

        int socket_fd_ = -1;

        std::vector<char> outbound_data_;
        size_t next_send_valid_index_ = 0;
        std::vector<char> inbound_data_;
        size_t next_rcv_valid_index_ = 0;

        struct sockaddr_in socket_attrib_{};

        std::function<void(TCPSocket *s, Nanos rx_time)> recv_callback_ = nullptr;

        std::string time_str_;
        Logger &logger_;
    };
}
