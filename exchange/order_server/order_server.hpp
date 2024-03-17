#pragma once

#include "common/thread_utils.h"
#include "common/tcp_server.hpp"
#include "common/types.hpp"
#include "common/macros.hpp"
#include "order_server/client_request.hpp"
#include "order_server/client_response.hpp"
#include "order_server/fifo_sequencer.hpp"

namespace Exchange {
class OrderServer {
private:
    const std::string iface_;
    const int port_ = 0;

    ClientResponseLFQueue* outgoing_responses_ = nullptr;

    volatile bool running_ = false;

    std::array<size_t, ME_MAX_NUM_CLIENTS> cid_next_outgoing_seq_num_;
    std::array<size_t, ME_MAX_NUM_CLIENTS> cid_next_exp_seq_num_;
    std::array<Common::TCPSocket *, ME_MAX_NUM_CLIENTS> cid_tcp_socket_;

    Common::TCPServer tcp_server_;
    FIFOSequencer fifo_sequencer_;

    std::string time_str_;
    Logger logger_;

    MEClientResponse me_client_response_;

public:
    OrderServer(const std::string& iface, int port, ClientResponseLFQueue* outgoing_responses);
    ~OrderServer();

    void start();
    void stop();

    void run() noexcept;

    void recvCallback(TCPSocket* socket, Nanos rx_time) noexcept;
    void recvFinishedCallback() noexcept;

    OrderServer() = delete;
    OrderServer(const OrderServer&) = delete;
    OrderServer(const OrderServer&&) = delete;
    OrderServer& operator=(const OrderServer&) = delete;
    OrderServer& operator=(const OrderServer&&) = delete;
};
}