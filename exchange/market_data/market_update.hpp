#pragma once

#include <sstream>

#include "common/types.hpp"
#include "common/lf_queue.hpp"

using namespace Common;

namespace Exchange {
    #pragma pack(push, 1)
    enum class MarketUpdateType : uint8_t {
        INVALID = 0,
        CLEAR = 1,
        ADD = 2,
        MODIFY = 3,
        CANCEL = 4,
        TRADE = 5,
        SNAPSHOT_START = 6,
        SNAPSHOT_END = 7
    };

    inline std::string marketUpdateTypeToString(MarketUpdateType type) {
        switch (type) {
        case MarketUpdateType::CLEAR:
            return "CLEAR";
        case MarketUpdateType::ADD:
            return "ADD";
        case MarketUpdateType::MODIFY:
            return "MODIFY";
        case MarketUpdateType::CANCEL:
            return "CANCEL";
        case MarketUpdateType::TRADE:
            return "TRADE";
        case MarketUpdateType::SNAPSHOT_START:
            return "SNAPSHOT_START";
        case MarketUpdateType::SNAPSHOT_END:
            return "SNAPSHOT_END";
        case MarketUpdateType::INVALID:
            return "INVALID";
        }

        return "UNKNOWN";
    }

    struct MEMarketUpdate {
        MarketUpdateType type_;
        OrderId order_id_ = OrderId_INVALID;
        TickerId ticker_id_ = TickerId_INVALID;
        Side side_ = Side::INVALID;
        Price price_ = Price_INVALID;
        Qty qty_ = Qty_INVALID;
        Priority priority_ = Priority_INVALID;

        auto toString() const {
            std::stringstream ss;
            ss << "MEMarketUpdate["
                << " type:" << marketUpdateTypeToString(type_)
                << " ticker:" << tickerIdToString(ticker_id_)
                << " oid:" << orderIdToString(order_id_)
                << " side:" << sideToString(side_)
                << " qty:" << qtyToString(qty_)
                << " price:" << priceToString(price_)
                << " priority:" << priorityToString(priority_)
                << "]";
            return ss.str();
        }
    };

        struct PubMarketUpdate {
        size_t seq_num_ = 0;
        MEMarketUpdate me_market_update_;

        std::toString() const noexcept {
            std::stringstream ss;
            ss << "PubMarketUpdate["
                << "seq:" << seq_num_
                << " " << me_market_update_.toString()
                << "]";
            return ss.str();
        }
    };

    #pragma pack(pop)

    typedef LFQueue<MEMarketUpdate> MEMarketUpdateLFQueue;
    typedef LFQueue<PubMarketUpdate> PubMarketUpdateLFQueue;
}