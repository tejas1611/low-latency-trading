#pragma once

#include <sstream>

#include "common/types.hpp"
#include "common/lf_queue.hpp"

using namespace Common;

namespace Exchange {
    #pragma pack(push, 1)
    enum class ClientRequestType : uint8_t {
        INVALID = 0,
        NEW = 1,
        CANCEL = 2,
    };

    inline std::string clientRequestTypeToString(ClientRequestType type) {
        switch (type) {
            case ClientRequestType::INVALID:
                return "INVALID";
            case ClientRequestType::NEW:
                return "NEW";
            case ClientRequestType::CANCEL:
                return "CANCEL";
        }

        return "UNKNOWN";
    }

    struct MEClientRequest {
        ClientRequestType type_;
        ClientId client_id_ = ClientId_INVALID;
        TickerId ticker_id_ = TickerId_INVALID;
        OrderId client_order_id_ = OrderId_INVALID;
        Side side_ = Side::INVALID;
        Price price_ = Price_INVALID;
        Qty qty_ = Qty_INVALID;

        auto toString() const {
            std::stringstream ss;
            ss << "MEClientRequest["
                << "type:" << clientRequestTypeToString(type_)
                << " client:" << clientIdToString(client_id_)
                << " ticker:" << tickerIdToString(ticker_id_)
                << " coid:" << orderIdToString(client_order_id_)
                << " side:" << sideToString(side_)
                << " qty:" << qtyToString(qty_)
                << " price:" << priceToString(price_)
                << "]";
            return ss.str();
        }
    };

    struct PubClientRequest {
        size_t seq_num_ = 0;
        MEClientRequest me_client_request_;

        std::toString() const noexcept {
            std::stringstream ss;
            ss << "PubClientRequest["
                << "seq:" << seq_num_
                << " " << me_client_request_.toString()
                << "]";
            return ss.str();
        }
    };

    #pragma pack(pop)

    typedef LFQueue<MEClientRequest> ClientRequestLFQueue;
}