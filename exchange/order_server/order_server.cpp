#include "order_server/order_server.hpp"

namespace Exchange {
    OrderServer::OrderServer(const std::string& iface, int port, ClientResponseLFQueue* outgoing_responses, ClientRequestLFQueue* incoming_requests)
        : iface_(iface), port_(port), outgoing_responses_(outgoing_responses), logger_("exchange_order_server.log"), 
        tcp_server_(logger_), fifo_sequencer_(incoming_requests, &logger_) {
        cid_next_outgoing_seq_num_.fill(1);
        cid_next_exp_seq_num_.fill(1);
        cid_tcp_socket_.fill(nullptr);

        tcp_server_.recv_callback_ = [this](auto socket, auto rx_time) { recvCallback(socket, rx_time); };
        tcp_server_.recv_finished_callback_ = [this]() { recvFinishedCallback(); };
    }

    OrderServer::~OrderServer() {
        stop();

        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(1s);
    }

    void OrderServer::start() {
        running_ = true;
        tcp_server_.listen(iface_, port_);

        ASSERT(createAndStartThread(-1, "Exchange/OrderServer", [this]() { run(); }), "Failed to start Exchange/OrderServer thread");
    }

    void OrderServer::stop() {
        running_ = false;
    }

    void OrderServer::run() noexcept {
        logger_.log("%:% %() %\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str_));
        while (running_) {
            tcp_server_.poll();
            tcp_server_.sendAndRecv();

            while (outgoing_responses_->pop(me_client_response_)) {
                auto &next_outgoing_seq_num = cid_next_outgoing_seq_num_[me_client_response_.client_id_];
                logger_.log("%:% %() % Processing cid:% seq:% %\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str_),
                            me_client_response_.client_id_, next_outgoing_seq_num, me_client_response_.toString());

                ASSERT(cid_tcp_socket_[me_client_response_.client_id_] != nullptr,
                        "Dont have a TCPSocket for ClientId:" + std::to_string(me_client_response_.client_id_));
                cid_tcp_socket_[me_client_response_.client_id_]->send(&next_outgoing_seq_num, sizeof(next_outgoing_seq_num));
                cid_tcp_socket_[me_client_response_.client_id_]->send(&me_client_response_, sizeof(MEClientResponse));

                ++next_outgoing_seq_num;
            }
        }
    }

    void OrderServer::recvCallback(TCPSocket* socket, Nanos rx_time) noexcept {
        logger_.log("%:% %() % Received socket:% len:% rx:%\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str_),
                  socket->socket_fd_, socket->next_rcv_valid_index_, rx_time);

        if (socket->next_rcv_valid_index_ >= sizeof(PubClientRequest)) {
            size_t i = 0;
            for (; i + sizeof(PubClientRequest) <= socket->next_rcv_valid_index_; i += sizeof(PubClientRequest)) {
                auto request = reinterpret_cast<const PubClientRequest *>(socket->inbound_data_.data() + i);
                
                logger_.log("%:% %() % Received %\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str_), request->toString());

                if (cid_tcp_socket_[request->me_client_request_.client_id_] == nullptr) [[unlikely]] { // first message from this ClientId.
                    cid_tcp_socket_[request->me_client_request_.client_id_] = socket;
                }

                if (cid_tcp_socket_[request->me_client_request_.client_id_] != socket) [[unlikely]] {   // mismatch socket
                    logger_.log("%:% %() % Received ClientRequest from ClientId:% on different socket:% expected:%\n", __FILE__, __LINE__, __FUNCTION__,
                                Common::getCurrentTimeStr(&time_str_), request->me_client_request_.client_id_, socket->socket_fd_,
                                cid_tcp_socket_[request->me_client_request_.client_id_]->socket_fd_);
                    MEClientResponse response {ClientResponseType::REJECTED, request->me_client_request_.client_id_, TickerId_INVALID, 
                                    OrderId_INVALID, OrderId_INVALID, Side::INVALID, Price_INVALID, Qty_INVALID, Qty_INVALID};
                    outgoing_responses_->push(response);
                }

                auto& next_exp_seq_num = cid_next_exp_seq_num_[request->me_client_request_.client_id_];
                if (request->seq_num_ != next_exp_seq_num) [[unlikely]] {                               // out of order sequence number
                    logger_.log("%:% %() % Incorrect sequence number. ClientId:% SeqNum expected:% received:%\n", __FILE__, __LINE__, __FUNCTION__,
                                Common::getCurrentTimeStr(&time_str_), request->me_client_request_.client_id_, next_exp_seq_num, request->seq_num_);
                    MEClientResponse response {ClientResponseType::REJECTED, request->me_client_request_.client_id_, TickerId_INVALID, 
                                    OrderId_INVALID, OrderId_INVALID, Side::INVALID, Price_INVALID, Qty_INVALID, Qty_INVALID};
                    outgoing_responses_->push(response);
                }

                ++next_exp_seq_num;

                fifo_sequencer_.addClientRequest(rx_time, request->me_client_request_);
            }
            memcpy(socket->inbound_data_.data(), socket->inbound_data_.data() + i, socket->next_rcv_valid_index_ - i);
            socket->next_rcv_valid_index_ -= i;
        }
    }

    void OrderServer::recvFinishedCallback() noexcept {
        fifo_sequencer_.sequenceAndPublish();
    }
}