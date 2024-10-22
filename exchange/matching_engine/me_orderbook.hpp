#pragma once

#include "common/mem_pool.hpp"
#include "common/logger.hpp"
#include "common/macros.hpp"
#include "common/types.hpp"
#include "market_data/market_update.hpp"
#include "order_server/client_response.hpp"

#include "matching_engine/me_order.hpp"

using namespace Common;

namespace Exchange {
    class MatchingEngine;

    class MEOrderBook final {
    private:
        TickerId ticker_id_ = TickerId_INVALID;

        MatchingEngine* matching_engine_ = nullptr;
        Logger* logger_ = nullptr;

        ClientOrderHashMap cid_oid_to_order_;

        MemPool<MEOrdersAtPrice> orders_at_price_pool_;
        MEOrdersAtPrice* bids_at_price_ = nullptr;
        MEOrdersAtPrice* asks_at_price_ = nullptr;

        OrdersAtPriceHashMap price_orders_at_price_;

        MemPool<MEOrder> order_pool_;

        MEClientResponse client_response_;
        MEMarketUpdate market_update_;
        
        OrderId next_order_id_ = 1;
        std::string time_str_;

    public:
        explicit MEOrderBook(TickerId ticker_id, MatchingEngine* matchine_engine, Logger* logger);
        ~MEOrderBook();

        std::string toString(bool detailed, bool validity_check) const;

        void add(ClientId client_id, OrderId client_order_id, Side side, Price price, Qty qty) noexcept;
        void cancel(ClientId client_id, OrderId client_order_id) noexcept;

        MEOrderBook() = delete;
        MEOrderBook(const MEOrderBook&) = delete;
        MEOrderBook(const MEOrderBook&&) = delete;
        MEOrderBook& operator=(const MEOrderBook&) = delete;
        MEOrderBook& operator=(const MEOrderBook&&) = delete;

    private:
        OrderId generateNewMarketOrderId() noexcept {
            return next_order_id_++;
        }

        auto priceToIndex(Price price) const {
            return (price % ME_MAX_PRICE_LEVELS);
        } 

        MEOrdersAtPrice* getOrdersAtPrice(Price price) const {
            return price_orders_at_price_[priceToIndex(price)];
        }

        void addOrdersAtPrice(MEOrdersAtPrice* new_orders_at_price);

        void removeOrdersAtPrice(Side side, Price price);

        Priority getNextPriority(Price price) noexcept {
            const auto orders_at_price = getOrdersAtPrice(price);
            if (!orders_at_price)
                return 1lu;

            return orders_at_price->first_order_->prev_order_->priority_ + 1;
        }

        void match(TickerId ticker_id, ClientId client_id, Side side, OrderId client_order_id, OrderId new_market_order_id, MEOrder* matched_order, Qty& leaves_qty) noexcept;

        Qty checkForMatch(ClientId client_id, OrderId client_order_id, TickerId ticker_id, Side side, Price price, Qty qty, OrderId new_market_order_id) noexcept;

        void addOrder(MEOrder* order) noexcept;

        void removeOrder(MEOrder* order) noexcept;
    };

    typedef std::array<MEOrderBook*, ME_MAX_TICKERS> OrderBookHashMap;
}