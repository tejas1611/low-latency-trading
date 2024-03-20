#pragma once

#include <sstream>

#include "common/types.hpp"

using namespace Common;

namespace Trading {
    struct MarketOrder {
        OrderId order_id_ = OrderId_INVALID;
        Side side_ = Side::INVALID;
        Price price_ = Price_INVALID;
        Qty qty_ = Qty_INVALID;
        Priority priority_ = Priority_INVALID;

        MarketOrder* prev_order_ = nullptr;
        MarketOrder* next_order_ = nullptr;

        MarketOrder() = default;

        MarketOrder(OrderId order_id, Side side, Price price, Qty qty, Priority priority, 
            MarketOrder* prev_order, MarketOrder* next_order)
            : order_id_(order_id), side_(side), price_(price), qty_(qty), priority_(priority), prev_order_(prev_order), next_order_(next_order) {}

        std::string toString() const {
            std::stringstream ss;
            ss << "MarketOrder["
                << "oid:" << orderIdToString(order_id_) << " "
                << "side:" << sideToString(side_) << " "
                << "price:" << priceToString(price_) << " "
                << "qty:" << qtyToString(qty_) << " "
                << "prio:" << priorityToString(priority_) << " "
                << "prev:" << orderIdToString(prev_order_ ? prev_order_->order_id_ : OrderId_INVALID) << " "
                << "next:" << orderIdToString(next_order_ ? next_order_->order_id_ : OrderId_INVALID) << "]";

            return ss.str();
        }
    };

    typedef std::array<MarketOrder*, ME_MAX_ORDER_IDS> OrderHashMap;

    struct MarketOrdersAtPrice {
        Side side_ = Side::INVALID;
        Price price_ = Price_INVALID;

        MarketOrder* first_order_ = nullptr;

        MarketOrdersAtPrice* prev_entry_ = nullptr;
        MarketOrdersAtPrice* next_entry_ = nullptr;

        MarketOrdersAtPrice() = default;

        MarketOrdersAtPrice(Side side, Price price, MarketOrder* first_order, MarketOrdersAtPrice* prev_entry, MarketOrdersAtPrice* next_entry)
            : side_(side), price_(price), first_order_(first_order), prev_entry_(prev_entry), next_entry_(next_entry) {}

        std::string toString() const {
            std::stringstream ss;
            ss << "MarketOrdersAtPrice["
                << "side:" << sideToString(side_) << " "
                << "price:" << priceToString(price_) << " "
                << "first_order:" << (first_order_ ? first_order_->toString() : "null") << " "
                << "prev:" << priceToString(prev_entry_ ? prev_entry_->price_ : Price_INVALID) << " "
                << "next:" << priceToString(next_entry_ ? next_entry_->price_ : Price_INVALID) << "]";

            return ss.str();
        }
    };

    typedef std::array<MarketOrdersAtPrice*, ME_MAX_PRICE_LEVELS> OrdersAtPriceHashMap;

    struct BBO {
        Price bid_price_ = Price_INVALID;
        Price ask_price_ = Price_INVALID;
        Qty bid_qty_ = Qty_INVALID;
        Qty ask_qty_ = Qty_INVALID;

        std::string toString() const {
            std::stringstream ss;
            ss << "BBO["
                << qtyToString(bid_qty_) << "@" << priceToString(bid_price_)
                << "X"
                << priceToString(ask_price_) << "@" << qtyToString(ask_qty_)
                << "]";

            return ss.str();
        }
    };
}