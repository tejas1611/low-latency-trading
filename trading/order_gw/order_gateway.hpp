#pragma once

#include <functional>

#include "common/thread_utils.h"
#include "common/macros.h"
#include "common/tcp_server.h"
#include "common/types.hpp"

#include "exchange/order_server/client_request.h"
#include "exchange/order_server/client_response.h"

namespace Trading {
    class OrderGateway {
    private:
        const ClientId client_id_;

        std::string ip_;
        const std::string iface_;
        const int port_ = 0;

        Exchange::ClientResponseLFQueue* incoming_responses_ = nullptr;
        Exchange::ClientRequestLFQueue* outgoing_requests_ = nullptr;

        volatile bool running_ = false;

        std::string time_str_;
        Logger logger_;

        Common::TCPSocket tcp_socket_;

        size_t next_outgoing_seq_num_ = 1;
        size_t next_exp_seq_num_ = 1;

        Exchange::MEClientRequest me_client_request_;
        
    public:
        OrderGateway(const ClientId client_id, const std::string& ip, const std::string& iface, int port, 
                    Exchange::ClientResponseLFQueue* incoming_responses,
                    Exchange::ClientRequestLFQueue* outgoing_requests);
        ~OrderGateway();

        void start();
        void stop();

        OrderGateway() = delete;
        OrderGateway(const OrderGateway&) = delete;
        OrderGateway(const OrderGateway&&) = delete;
        OrderGateway& operator=(const OrderGateway&) = delete;
        OrderGateway& operator=(const OrderGateway&&) = delete;

    private:
        void run() noexcept;
        void recvCallback(TCPSocket* socket, Nanos rx_time) noexcept;
    };
}