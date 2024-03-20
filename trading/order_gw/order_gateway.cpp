#include "order_gw/order_gateway.hpp"

namespace Trading {
    OrderGateway::OrderGateway(const ClientId client_id, const std::string& ip, const std::string& iface, int port, 
        Exchange::ClientResponseLFQueue* incoming_responses,
        Exchange::ClientRequestLFQueue* outgoing_requests) : 
        client_id_(client_id), ip_(ip), iface_(iface), port_(port), 
        incoming_responses_(incoming_responses), outgoing_requests_(outgoing_requests) {
            tcp_socket_.recv_callback_ = [this](auto socket, auto rx_time) { recvCallback(socket, rx_time); };

    }
    
    OrderGateway::~OrderGateway() {
        stop();

        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(5s);
    }

    void OrderGateway::start() {
        running_ = true;
        ASSERT(tcp_socket_.connect(ip_, iface_, port_, false) >= 0,
            "Unable to connect to ip:" + ip_ + " port:" + std::to_string(port_) + " on iface:" + iface_ + " error:" + std::string(std::strerror(errno)));
        ASSERT(Common::createAndStartThread(-1, "Trading/OrderGateway", [this]() { run(); }), 
            "Failed to start Trading/OrderGateway thread");
    }

    void OrderGateway::stop() {
        running_ = false;
    }

    void OrderGateway::run() noexcept {
        logger_.log("%:% %() %\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str_));
        while (running_) {
            tcp_socket_.sendAndRecv();

            while (outgoing_requests_->pop(me_client_request_)) {
                logger_.log("%:% %() % Sending cid:% seq:% %\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str_),
                            me_client_request_.client_id_, next_outgoing_seq_num_, me_client_request_.toString());

                tcp_socket_->send(&next_outgoing_seq_num_, sizeof(next_outgoing_seq_num_));
                tcp_socket_->send(&me_client_request_, sizeof(Exchange::MEClientResponse));

                ++next_outgoing_seq_num_;
            }
        }
    }

    void OrderGateway::recvCallback(TCPSocket* socket, Nanos rx_time) noexcept {
        logger_.log("%:% %() % Received socket:% len:% rx:%\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str_),
                  socket->socket_fd_, socket->next_rcv_valid_index_, rx_time);

        if (socket->next_rcv_valid_index_ >= sizeof(Exchange::PubClientResponse)) {
            size_t i = 0;
            for (; i + sizeof(Exchange::PubClientResponse) <= socket->next_rcv_valid_index_; i += sizeof(Exchange::PubClientResponse)) {
                auto response = reinterpret_cast<const Exchange::PubClientResponse*>(socket->inbound_data_.data() + i);
                
                logger_.log("%:% %() % Received %\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str_), response->toString());

                if (response->me_client_response_.client_id_ != client_id_) [[unlikely]] {   // mismatch client_id
                    logger_.log("%:% %() % Received ClientResponse for different ClientId:% expected:%\n", __FILE__, __LINE__, __FUNCTION__,
                                Common::getCurrentTimeStr(&time_str_), response->me_client_response_.client_id_, client_id_);
                    continue;
                }

                if (response->seq_num_ != next_exp_seq_num_) [[unlikely]] {                               // out of order sequence number
                    logger_.log("%:% %() % Incorrect sequence number. ClientId:% SeqNum expected:% received:%\n", __FILE__, __LINE__, __FUNCTION__,
                                Common::getCurrentTimeStr(&time_str_), response->me_client_response_.client_id_, next_exp_seq_num, response->seq_num_);
                    continue;
                }

                ++next_exp_seq_num;
                incoming_responses_->push(std::move(response->me_client_response_));
            }
            
            memcpy(socket->inbound_data_.data(), socket->inbound_data_.data() + i, socket->next_rcv_valid_index_ - i);
            socket->next_rcv_valid_index_ -= i;
        }
    }
}