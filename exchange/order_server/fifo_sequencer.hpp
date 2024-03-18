#pragma once

#include "common/time_utils.hpp"
#include "common/types.hpp"

#include "order_server/client_request.hpp"

namespace Exchange
{
constexpr size_t ME_MAX_PENDING_REQUESTS = 1024;
class FIFOSequencer {
private:
    ClientRequestLFQueue* incoming_requests_ = nullptr;

    std::string time_str_;
    Logger* logger_ = nullptr;

    struct RecvTimeClientRequest {
        Nanos recv_time_;
        MEClientRequest me_client_request_;

        bool operator<(const RecvTimeClientRequest& other) {
            return this->recv_time_ < other.recv_time_;
        }
    };

    std::array<RecvTimeClientRequest, ME_MAX_PENDING_REQUESTS> pending_client_requests_;
    size_t pending_size_ = 0;

public:
    FIFOSequencer(ClientRequestLFQueue* incoming_requests, Logger* logger) : incoming_requests_(incoming_requests), logger_(logger) {}
    ~FIFOSequencer() {
        logger_ = nullptr;
        incoming_requests_ = nullptr;
    }

    void addClientRequest(Nanos rx_time, const MEClientRequest &request) {
        ASSERT(pending_size_ < pending_client_requests_.size(), "ME FIFOSequencer: Too many pending requests");
        pending_client_requests_[pending_size_++] = std::move(RecvTimeClientRequest{rx_time, request});
    }

    void sequenceAndPublish() {
        if (pending_size_ == 0) [[unlikely]] {
            return;
        }

        logger_->log("%:% %() % Processing % requests.\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str_), pending_size_);

        std::sort(pending_client_requests_.begin(), pending_client_requests_.begin() + pending_size_);

        for (size_t i = 0; i < pending_size_; ++i) {
            const auto &client_request = pending_client_requests_.at(i);

            logger_->log("%:% %() % Writing RX:% Req:% to FIFO.\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str_),
                        client_request.recv_time_, client_request.me_client_request_.toString());
                        
            incoming_requests_->push(std::move(client_request.me_client_request_));
        }

        pending_size_ = 0;
    }

    FIFOSequencer() = delete;
    FIFOSequencer(const FIFOSequencer&) = delete;
    FIFOSequencer(const FIFOSequencer&&) = delete;
    FIFOSequencer& operator=(const FIFOSequencer&) = delete;
    FIFOSequencer& operator=(const FIFOSequencer&&) = delete;
};
}
