#pragma once

#include "common/mem_pool.hpp"
#include "common/logger.hpp"
#include "common/macros.hpp"
#include "common/types.hpp"
#include "strategy/market_order.hpp"

#include "exchange/market_data/market_update.h"

using namespace Common;

namespace Trading {
    class TradingEngine;

    class MarketOrderBook final {
    private:
        TickerId ticker_id_ = TickerId_INVALID;

        TradingEngine* trading_engine_ = nullptr;

        OrderHashMap oid_to_order_;

        MemPool<MarketOrdersAtPrice> orders_at_price_pool_;
        MarketOrdersAtPrice* bids_at_price_ = nullptr;
        MarketOrdersAtPrice* asks_at_price_ = nullptr;

        OrdersAtPriceHashMap price_orders_at_price_;
        MemPool<MarketOrder> order_pool_;
        BBO bbo_;
        
        std::string time_str_;
        Logger* logger_ = nullptr;

    public:
        explicit MarketOrderBook(TickerId ticker_id, TradingEngine* trading_engine_, Logger* logger);
        ~MarketOrderBook();

        const BBO* getBBO() const {
            return &bbo_;
        }

        std::string toString(bool detailed, bool validity_check) const;

        void updateBBO(bool update_bid, bool update_ask) noexcept;
        void onMarketUpdate(const Exchange::MEMarketUpdate *market_update) noexcept;

        void setTradeEngine(TradeEngine* trade_engine) {
            trading_engine_ = trade_engine;
        }

        MarketOrderBook() = delete;
        MarketOrderBook(const MarketOrderBook&) = delete;
        MarketOrderBook(const MarketOrderBook&&) = delete;
        MarketOrderBook& operator=(const MarketOrderBook&) = delete;
        MarketOrderBook& operator=(const MarketOrderBook&&) = delete;

    private:
        auto priceToIndex(Price price) const {
            return (price % ME_MAX_PRICE_LEVELS);
        } 

        MarketOrdersAtPrice* getOrdersAtPrice(Price price) const {
            return price_orders_at_price_[priceToIndex(price)];
        }

        void addOrdersAtPrice(MarketOrdersAtPrice* new_orders_at_price);
        void removeOrdersAtPrice(Side side, Price price);

        void addOrder(MarketOrder* order) noexcept;
        void removeOrder(MarketOrder* order) noexcept;
    };

    typedef std::array<MarketOrderBook*, ME_MAX_TICKERS> MarketOrderBookHashMap;
}