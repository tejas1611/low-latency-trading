#include "matching_engine/me_orderbook.hpp"

#include "matching_engine/matching_engine.hpp"

namespace Exchange {

MEOrderBook::MEOrderBook(TickerId ticker_id, MatchingEngine* matchine_engine, Logger* logger) :
ticker_id_(ticker_id), matching_engine_(matchine_engine), logger_(logger), 
orders_at_price_pool_(ME_MAX_PRICE_LEVELS), order_pool_(ME_MAX_ORDER_IDS) { }

MEOrderBook::~MEOrderBook() {
    logger_->log("%:% %() % OrderBook\n%\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str_),
                toString(false, true));
    
    matching_engine_ = nullptr;
    bids_at_price_ = nullptr;
    asks_at_price_ = nullptr;
    logger_ = nullptr;
    for (auto& it: cid_oid_to_order_) {
        it.fill(nullptr);
    }
}

void MEOrderBook::add(ClientId client_id, OrderId client_order_id, Side side, Price price, Qty qty) noexcept {
    const OrderId new_market_order_id = generateNewMarketOrderId();
    client_response_ = {ClientResponseType::ACCEPTED, client_id, ticker_id_, client_order_id, new_market_order_id, side, price, 0, qty};
    matching_engine_->sendClientResponse(client_response_);

    const Qty leaves_qty = checkForMatch(client_id, client_order_id, ticker_id_, side, price, qty, new_market_order_id);

    if (leaves_qty > 0) [[likely]] {
        const Priority priority = getNextPriority(price);

        MEOrder* order = order_pool_.allocate(ticker_id_, client_order_id, new_market_order_id, client_id, side, price, leaves_qty, priority, nullptr, nullptr);

        addOrder(order);

        market_update_ = {MarketUpdateType::ADD, new_market_order_id, ticker_id_, side, price, leaves_qty, priority};
        matching_engine_->sendMarketUpdate(market_update_);
    }
}

void MEOrderBook::cancel(ClientId client_id, OrderId client_order_id) noexcept {
    bool is_cancelable = (client_order_id < cid_oid_to_order_.size());
    MEOrder* order = nullptr;

    if (is_cancelable) [[likely]] {
        auto& it = cid_oid_to_order_.at(client_id);
        order = it.at(client_order_id);
        is_cancelable = (order != nullptr);
    }

    if (is_cancelable) [[likely]] {
        client_response_ = {ClientResponseType::CANCELED, client_id, ticker_id_, client_order_id, order->market_order_id_, 
        order->side_, order->price_, 0, order->qty_};
        market_update_ = {MarketUpdateType::CANCEL, order->market_order_id_, ticker_id_, order->side_, order->price_, order->qty_, order->priority_};
        matching_engine_->sendMarketUpdate(market_update_);
    }
    else {
        client_response_ = {ClientResponseType::CANCEL_REJECTED, client_id, ticker_id_, client_order_id, OrderId_INVALID, 
        Side::INVALID, Price_INVALID, Qty_INVALID, Qty_INVALID};
    }

    matching_engine_->sendClientResponse(client_response_);
}

// Function referred from author's implementation at https://github.com/PacktPublishing/Building-Low-Latency-Applications-with-CPP/blob/fc7061f3435009a5e8d78b2dc189c50b59317d58/Chapter6/exchange/matcher/me_order_book.cpp#L126
std::string MEOrderBook::toString(bool detailed, bool validity_check) const {
    std::stringstream ss;
    std::string time_str;

    auto printer = [&](std::stringstream &ss, MEOrdersAtPrice *itr, Side side, Price &last_price, bool sanity_check) {
        char buf[4096];
        Qty qty = 0;
        size_t num_orders = 0;

        for (auto o_itr = itr->first_order_;; o_itr = o_itr->next_order_) {
        qty += o_itr->qty_;
        ++num_orders;
        if (o_itr->next_order_ == itr->first_order_)
            break;
        }
        sprintf(buf, " <px:%3s p:%3s n:%3s> %-3s @ %-5s(%-4s)",
                priceToString(itr->price_).c_str(), priceToString(itr->prev_entry_->price_).c_str(), priceToString(itr->next_entry_->price_).c_str(),
                priceToString(itr->price_).c_str(), qtyToString(qty).c_str(), std::to_string(num_orders).c_str());
        ss << buf;
        for (auto o_itr = itr->first_order_;; o_itr = o_itr->next_order_) {
        if (detailed) {
            sprintf(buf, "[oid:%s q:%s p:%s n:%s] ",
                    orderIdToString(o_itr->market_order_id_).c_str(), qtyToString(o_itr->qty_).c_str(),
                    orderIdToString(o_itr->prev_order_ ? o_itr->prev_order_->market_order_id_ : OrderId_INVALID).c_str(),
                    orderIdToString(o_itr->next_order_ ? o_itr->next_order_->market_order_id_ : OrderId_INVALID).c_str());
            ss << buf;
        }
        if (o_itr->next_order_ == itr->first_order_)
            break;
        }

        ss << std::endl;

        if (sanity_check) {
        if ((side == Side::SELL && last_price >= itr->price_) || (side == Side::BUY && last_price <= itr->price_)) {
            FATAL("Bids/Asks not sorted by ascending/descending prices last:" + priceToString(last_price) + " itr:" + itr->toString());
        }
        last_price = itr->price_;
        }
    };

    ss << "Ticker:" << tickerIdToString(ticker_id_) << std::endl;
    {
      auto ask_itr = asks_at_price_;
      auto last_ask_price = std::numeric_limits<Price>::min();
      for (size_t count = 0; ask_itr; ++count) {
        ss << "ASKS L:" << count << " => ";
        auto next_ask_itr = (ask_itr->next_entry_ == asks_at_price_ ? nullptr : ask_itr->next_entry_);
        printer(ss, ask_itr, Side::SELL, last_ask_price, validity_check);
        ask_itr = next_ask_itr;
      }
    }

    ss << std::endl << "                          X" << std::endl << std::endl;

    {
      auto bid_itr = bids_at_price_;
      auto last_bid_price = std::numeric_limits<Price>::max();
      for (size_t count = 0; bid_itr; ++count) {
        ss << "BIDS L:" << count << " => ";
        auto next_bid_itr = (bid_itr->next_entry_ == bids_at_price_ ? nullptr : bid_itr->next_entry_);
        printer(ss, bid_itr, Side::BUY, last_bid_price, validity_check);
        bid_itr = next_bid_itr;
      }
    }

    return ss.str();
}

void MEOrderBook::addOrdersAtPrice(MEOrdersAtPrice* new_orders_at_price) {
    price_orders_at_price_.at(priceToIndex(new_orders_at_price->price_)) = new_orders_at_price;
    const Side side = new_orders_at_price->side_;

    MEOrdersAtPrice* orders_at_price_it = (side == Side::BUY ? bids_at_price_ : asks_at_price_);
    if (orders_at_price_it == nullptr) [[unlikely]] {
        new_orders_at_price->next_entry_ = new_orders_at_price->prev_entry_ = new_orders_at_price;
        (side == Side::BUY ? bids_at_price_ : asks_at_price_) = new_orders_at_price;
    }
    else {
        MEOrdersAtPrice* const initial_it = orders_at_price_it;     // initial_it used to check loop around of orders_at_price_it
        MEOrdersAtPrice* add_after_it = nullptr;

        while (orders_at_price_it->next_entry_ != initial_it) {
            if ((side == Side::BUY && orders_at_price_it->price_ < new_orders_at_price->price_) || 
                (side == Side::SELL && orders_at_price_it->price_ > new_orders_at_price->price_)) break;
            
            add_after_it = orders_at_price_it;
            orders_at_price_it = orders_at_price_it->next_entry_;
        }

        if (add_after_it == nullptr) {
            /* Two possibilities:
                1. There is only one price level and new_orders_at_price is inserted before/after this
                2. There are >1 price levels and new_orders_at_price is inserted at start of linked list
            */
            if (orders_at_price_it->next_entry_ == initial_it) {
                new_orders_at_price->next_entry_ = new_orders_at_price->prev_entry_ = orders_at_price_it;
                orders_at_price_it->prev_entry_ = orders_at_price_it->next_entry_ = new_orders_at_price;
                if ((side == Side::BUY && orders_at_price_it->price_ < new_orders_at_price->price_) || 
                    (side == Side::SELL && orders_at_price_it->price_ > new_orders_at_price->price_)) {
                        (side == Side::BUY ? bids_at_price_ : asks_at_price_) = new_orders_at_price;
                    }
            }
            else {
                new_orders_at_price->prev_entry_ = orders_at_price_it->prev_entry_;
                new_orders_at_price->next_entry_ = orders_at_price_it;
                orders_at_price_it->prev_entry_ = new_orders_at_price;
                new_orders_at_price->prev_entry_->next_entry_ = new_orders_at_price;

                (side == Side::BUY ? bids_at_price_ : asks_at_price_) = new_orders_at_price;
            }
        } 
        else {
            new_orders_at_price->prev_entry_ = add_after_it;
            new_orders_at_price->next_entry_ = add_after_it->next_entry_;
            add_after_it->next_entry_ = new_orders_at_price;
            new_orders_at_price->next_entry_->prev_entry_ = new_orders_at_price;
        }
    }
}

void MEOrderBook::removeOrdersAtPrice(Side side, Price price) {
    MEOrdersAtPrice* best_orders_of_side = (side == Side::BUY ? bids_at_price_ : asks_at_price_);
    MEOrdersAtPrice* orders_at_price = getOrdersAtPrice(price);

    if (best_orders_of_side->next_entry_ == best_orders_of_side) {
        (side == Side::BUY ? bids_at_price_ : asks_at_price_) = nullptr;
    }
    else {
        orders_at_price->prev_entry_->next_entry_ = orders_at_price->next_entry_;
        orders_at_price->next_entry_->prev_entry_ = orders_at_price->prev_entry_;

        if (best_orders_of_side == orders_at_price) {
            (side == Side::BUY ? bids_at_price_ : asks_at_price_) = orders_at_price->next_entry_;
        }
    }

    orders_at_price->prev_entry_ = orders_at_price->next_entry_ = nullptr;
    price_orders_at_price_.at(priceToIndex(price)) = nullptr;

    orders_at_price_pool_.deallocate(orders_at_price);
}

void MEOrderBook::match(TickerId ticker_id, ClientId client_id, Side side, OrderId client_order_id, OrderId new_market_order_id, MEOrder* matched_order, Qty& leaves_qty) noexcept {
    const auto exec_qty = std::min(leaves_qty, matched_order->qty_);
    const auto exec_price = matched_order->price_;

    leaves_qty -= exec_qty;
    matched_order->qty_ -= exec_qty;

    client_response_ = {ClientResponseType::FILLED, client_id, ticker_id, client_order_id, new_market_order_id, side, exec_price, exec_qty, leaves_qty};
    matching_engine_->sendClientResponse(client_response_);

    client_response_ = {ClientResponseType::FILLED, matched_order->client_id_, ticker_id, matched_order->client_order_id_, 
                        matched_order->market_order_id_, matched_order->side_, exec_price, exec_qty, matched_order->qty_};
    matching_engine_->sendClientResponse(client_response_);

    market_update_ = {MarketUpdateType::TRADE, OrderId_INVALID, ticker_id_, side, exec_price, exec_qty, Priority_INVALID};
    matching_engine_->sendMarketUpdate(market_update_);

    if (matched_order->qty_ > 0) {
        market_update_ = {MarketUpdateType::MODIFY, matched_order->market_order_id_, ticker_id_, matched_order->side_, exec_price, matched_order->qty_, matched_order->priority_};
        matching_engine_->sendMarketUpdate(market_update_);
    }
    else {
        market_update_ = {MarketUpdateType::CANCEL, matched_order->market_order_id_, ticker_id_, matched_order->side_, exec_price, matched_order->qty_, Priority_INVALID};
        matching_engine_->sendMarketUpdate(market_update_);

        removeOrder(matched_order);
    }
}

Qty MEOrderBook::checkForMatch(ClientId client_id, OrderId client_order_id, TickerId ticker_id, Side side, Price price, Qty qty, OrderId new_market_order_id) noexcept {
    Qty leaves_qty = qty;
    switch (side) {
        case Side::BUY:
            while (leaves_qty > 0 && asks_at_price_ != nullptr) {
                MEOrder* top_ask = asks_at_price_->first_order_;

                if (top_ask->price_ > price) break;

                match(ticker_id, client_id, side, client_order_id, new_market_order_id, top_ask, leaves_qty);
            }
        break;

        case Side::SELL:
            while (leaves_qty > 0 && bids_at_price_ != nullptr) {
                MEOrder* top_bid = bids_at_price_->first_order_;

                if (top_bid->price_ < price) break;

                match(ticker_id, client_id, side, client_order_id, new_market_order_id, top_bid, leaves_qty);
            }
        break;
        
        default:
            FATAL("Received invalid client-request-side:" + sideToString(side));
    }

    return leaves_qty;
}

void MEOrderBook::addOrder(MEOrder* order) noexcept {
    MEOrdersAtPrice* orders_at_price = getOrdersAtPrice(order->price_);

    if (orders_at_price == nullptr) {
        order->next_order_ = order->prev_order_ = order;

        MEOrdersAtPrice* new_orders_at_price = orders_at_price_pool_.allocate(order->side_, order->price_, order, nullptr, nullptr);
        addOrdersAtPrice(new_orders_at_price);
    }
    else {
        auto first_order = orders_at_price->first_order_;
        first_order->prev_order_->next_order_ = order;
        order->prev_order_ = first_order->prev_order_;
        first_order->prev_order_ = order;
        order->next_order_ = first_order;
    }

    cid_oid_to_order_.at(order->client_id_).at(order->client_order_id_) = order;
}

void MEOrderBook::removeOrder(MEOrder* order) noexcept {
    MEOrdersAtPrice* orders_at_price = getOrdersAtPrice(order->price_);

    if (order->next_order_ == order) {   // Only order at the price level
        removeOrdersAtPrice(orders_at_price->side_, orders_at_price->price_);
    }
    else {
        order->prev_order_->next_order_ = order->next_order_;
        order->next_order_->prev_order_ = order->prev_order_;
        
        if (orders_at_price->first_order_ == order) {
            orders_at_price->first_order_ = order->next_order_;
        }
    }

    order->next_order_ = order->prev_order_ = nullptr;
    cid_oid_to_order_.at(order->client_id_).at(order->client_order_id_) = nullptr;
    order_pool_.deallocate(order);
}

}


