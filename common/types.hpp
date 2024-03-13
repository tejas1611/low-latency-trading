#pragma once

#include <cstdint>
#include <limits>
#include <string>

#include "macros.hpp"

namespace Common {
    constexpr size_t ME_MAX_TICKERS = 8;
    
    constexpr size_t ME_MAX_CLIENT_UPDATES = 256 * 1024;
    constexpr size_t ME_MAX_MARKET_UPDATES = 256 * 1024;

    constexpr size_t ME_MAX_NUM_CLIENTS = 256;
    constexpr size_t ME_MAX_ORDER_IDS = 1024 * 1024;
    constexpr size_t ME_MAX_PRICE_LEVELS = 256;
    
    typedef uint32_t TickerId;
    constexpr TickerId TickerId_INVALID = std::numeric_limits<TickerId>::max();

    inline std::string tickerIdToString(TickerId ticker_id) {
        if (ticker_id == TickerId_INVALID) [[unlikely]] {
            return "INVALID";
        }
        
        return std::to_string(ticker_id);
    }

    typedef uint64_t OrderId;
    constexpr OrderId OrderId_INVALID = std::numeric_limits<OrderId>::max();

    inline std::string orderIdToString(OrderId order_id) {
        if (order_id == OrderId_INVALID) [[unlikely]] {
            return "INVALID";
        }
        
        return std::to_string(order_id);
    }
    
    typedef uint32_t ClientId;
    constexpr ClientId ClientId_INVALID = std::numeric_limits<ClientId>::max();

    inline std::string clientIdToString(ClientId client_id) {
        if (client_id == ClientId_INVALID) [[unlikely]] {
            return "INVALID";
        }
        
        return std::to_string(client_id);
    }

    enum class Side : uint8_t {
        INVALID = 0,
        BUY = 1,
        SELL = -1
    };

    inline std::string sideToString(Side side) {
        switch (side) {
            case Side::INVALID:
                return "INVALID";
            case Side::BUY:
                return "BUY";
            case Side::SELL:
                return "SELL";
        }

        return "UNKNOWN";
    }

    typedef uint64_t Price;
    constexpr Price Price_INVALID = std::numeric_limits<Price>::max();

    inline std::string priceToString(Price price) {
        if (price == Price_INVALID) [[unlikely]] {
            return "INVALID";
        }
        
        return std::to_string(price);
    }

    typedef uint32_t Qty;
    constexpr Qty Qty_INVALID = std::numeric_limits<Qty>::max();

    inline std::string qtyToString(Qty qty) {
        if (qty == Qty_INVALID) [[unlikely]] {
            return "INVALID";
        }
        
        return std::to_string(qty);
    }

    typedef uint64_t Priority;
    constexpr Priority Priority_INVALID = std::numeric_limits<Priority>::max();

    inline std::string priorityToString(Priority priority) {
        if (priority == Priority_INVALID) [[unlikely]] {
            return "INVALID";
        }
        
        return std::to_string(priority);
    }
}