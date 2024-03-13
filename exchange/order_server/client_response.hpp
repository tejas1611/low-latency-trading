#pragma once

#include <sstream>

#include "common/types.hpp"
#include "common/lf_queue.hpp"

using namespace Common;

namespace Exchange {
    #pragma pack(push, 1)
    enum class ClientResponseType : uint8_t {
        INVALID = 0,
        ACCEPTED = 1,
        CANCELED = 2,
        FILLED = 3,
        CANCEL_REJECTED = 4
    };

    inline std::string clientResponseTypeToString(ClientResponseType type) {
        switch (type) {
            case ClientResponseType::INVALID:
                return "INVALID";
            case ClientResponseType::ACCEPTED:
                return "ACCEPTED";
            case ClientResponseType::CANCELED:
                return "CANCELED";
            case ClientResponseType::FILLED:
                return "FILLED";
            case ClientResponseType::CANCEL_REJECTED:
                return "CANCELED_REJECTED";
        }

        return "UNKNOWN";
    }

    struct MEClientResponse {
        ClientResponseType type_;
        ClientId client_id_ = ClientId_INVALID;
        TickerId ticker_id_ = TickerId_INVALID;
        OrderId client_order_id_ = OrderId_INVALID;
        OrderId market_order_id_ = OrderId_INVALID;
        Side side_ = Side::INVALID;
        Price price_ = Price_INVALID;
        Qty exec_qty_ = Qty_INVALID;
        Qty leaves_qty_ = Qty_INVALID;

        auto toString() const {
            std::stringstream ss;
            ss << "MEClientResponse["
                << "type:" << clientResponseTypeToString(type_)
                << " client:" << clientIdToString(client_id_)
                << " ticker:" << tickerIdToString(ticker_id_)
                << " coid:" << orderIdToString(client_order_id_)
                << " moid:" << orderIdToString(market_order_id_)
                << " side:" << sideToString(side_)
                << " exec_qty:" << qtyToString(exec_qty_)
                << " leaves_qty:" << qtyToString(leaves_qty_)
                << " price:" << priceToString(price_)
                << "]";
            return ss.str();
        }
    };
    #pragma pack(pop)

    typedef LFQueue<MEClientResponse> ClientResponseLFQueue;
}