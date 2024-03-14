#include "matching_engine/me_orderbook.hpp"

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

        MEOrder* order = order_pool_.allocate(ticker_id_, client_order_id, new_market_order_id, client_id, side, price, leaves_qty, priority);

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

std::string MEOrderBook::toString(bool detailed, bool validity_check) {

}

void MEOrderBook::addOrdersAtPrice(MEOrdersAtPrice* new_orders_at_price) {

}

void MEOrderBook::removeOrdersAtPrice(MEOrdersAtPrice* new_orders_at_price) {

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
                const MEOrder* top_ask = asks_at_price_->first_order_;

                if (top_ask->price_ > price) break;

                match(ticker_id, client_id, side, client_order_id, new_market_order_id, top_ask, leaves_qty);
            }
        break;

        case Side::SELL:
            while (leaves_qty > 0 && bids_at_price_ != nullptr) {
                const MEOrder* top_bid = bids_at_price_->first_order_;

                if (top_bid->price_ < price) break;

                match(ticker_id, client_id, side, client_order_id, new_market_order_id, top_bid, leaves_qty);
            }
        break;
    }

    return leaves_qty;
}

void MEOrderBook::addOrder(MEOrder* order) noexcept {

}

void MEOrderBook::removeOrder(MEOrder* order) noexcept {

}

}


