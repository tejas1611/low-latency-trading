#include "strategy/market_orderbook.hpp"

namespace Trading {
    void MarketOrderBook::addOrdersAtPrice(MarketOrdersAtPrice* new_orders_at_price) {
        price_orders_at_price_.at(priceToIndex(new_orders_at_price->price_)) = new_orders_at_price;
        const Side side = new_orders_at_price->side_;

        MarketOrdersAtPrice* orders_at_price_it = (side == Side::BUY ? bids_at_price_ : asks_at_price_);
        if (orders_at_price_it == nullptr) [[unlikely]] {
            new_orders_at_price->next_entry_ = new_orders_at_price->prev_entry_ = new_orders_at_price;
            (side == Side::BUY ? bids_at_price_ : asks_at_price_) = new_orders_at_price;
        }
        else {
            MarketOrdersAtPrice* const initial_it = orders_at_price_it;     // initial_it used to check loop around of orders_at_price_it
            MarketOrdersAtPrice* add_after_it = nullptr;

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

    void MarketOrderBook::removeOrdersAtPrice(Side side, Price price) {
        MarketOrdersAtPrice* best_orders_of_side = (side == Side::BUY ? bids_at_price_ : asks_at_price_);
        MarketOrdersAtPrice* orders_at_price = getOrdersAtPrice(price);

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

    void MarketOrderBook::addOrder(MarketOrder* order) noexcept {
        MarketOrdersAtPrice* orders_at_price = getOrdersAtPrice(order->price_);

        if (orders_at_price == nullptr) {
            order->next_order_ = order->prev_order_ = order;

            MarketOrdersAtPrice* new_orders_at_price = orders_at_price_pool_.allocate(order->side_, order->price_, order, nullptr, nullptr);
            addOrdersAtPrice(new_orders_at_price);
        }
        else {
            auto first_order = orders_at_price->first_order_;
            first_order->prev_order_->next_order_ = order;
            order->prev_order_ = first_order->prev_order_;
            first_order->prev_order_ = order;
            order->next_order_ = first_order;
        }

        oid_to_order_.at(order->order_id_) = order;
    }

    void MarketOrderBook::removeOrder(MarketOrder* order) noexcept {
        MarketOrdersAtPrice* orders_at_price = getOrdersAtPrice(order->price_);

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
        oid_to_order_.at(order->order_id_) = nullptr;
        order_pool_.deallocate(order);
    }

    void MarketOrderBook::updateBBO(bool update_bid, bool update_ask) noexcept {

    }
    
    void MarketOrderBook::onMarketUpdate(const Exchange::MEMarketUpdate* market_update) noexcept {
        const bool bid_updated = (bids_at_price_ && market_update->side_ == Side::BUY && market_update->price_ >= bids_at_price_->price_);
        const bool ask_updated = (asks_at_price_ && market_update->side_ == Side::SELL && market_update->price_ <= asks_at_price_->price_);

        switch (market_update->type) {
        case Exchange::MarketUpdateType::ADD: {
            MarketOrder* order = order_pool_.allocate(market_update->order_id_, market_update->side_, market_update->price_, 
                market_update->qty_, market_update->priority_, nullptr, nullptr);
            addOrder(order);
        }
        break;
        case Exchange::MarketUpdateType::MODIFY: {
            MarketOrder* order = oid_to_order_[market_update->order_id_];
            order->qty_ = market_update->qty_;
        }
        break;
        case Exchange::MarketUpdateType::CANCEL: {
            MarketOrder* order = oid_to_order_[market_update->order_id_];
            removeOrder(order);
        }
        break;
        case Exchange::MarketUpdateType::TRADE: 
            trading_engine_->onTradeUpdate(market_update, this);
        break;
        case Exchange::MarketUpdateType::CLEAR: {
            for (auto &order: oid_to_order_) {
                if (order)
                    order_pool_.deallocate(order);
            }
            oid_to_order_.fill(nullptr);

            if(bids_at_price_) {
                for(auto bid = bids_at_price_->next_entry_; bid != bids_at_price_; bid = bid->next_entry_)
                    orders_at_price_pool_.deallocate(bid);
                orders_at_price_pool_.deallocate(bids_at_price_);
            }

            if(asks_at_price_) {
                for(auto ask = asks_at_price_->next_entry_; ask != asks_at_price_; ask = ask->next_entry_)
                    orders_at_price_pool_.deallocate(ask);
                orders_at_price_pool_.deallocate(asks_at_price_);
            }

            bids_at_price_ = asks_at_price_ = nullptr;
        } 
        break;
        case Exchange::MarketUpdateType::INVALID:
        case Exchange::MarketUpdateType::SNAPSHOT_START:
        case Exchange::MarketUpdateType::SNAPSHOT_END:
        break;
        }

        updateBBO(bid_updated, ask_updated);

        logger_->log("%:% %() % % %", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str_), market_update->toString(), bbo_.toString());

        trading_engine_->onOrderBookUpdate(market_update->ticker_id_, market_update->price_, market_update->side_, this);
    }

    std::string MarketOrderBook::toString(bool detailed, bool validity_check) const {
        std::stringstream ss;
        std::string time_str;

        auto printer = [&](std::stringstream &ss, MarketOrdersAtPrice *itr, Side side, Price &last_price, bool sanity_check) {
            char buf[4096];
            Qty qty = 0;
            size_t num_orders = 0;

            for (auto o_itr = itr->first_mkt_order_;; o_itr = o_itr->next_order_) {
                qty += o_itr->qty_;
                ++num_orders;
                if (o_itr->next_order_ == itr->first_mkt_order_)
                break;
            }

            sprintf(buf, " <px:%3s p:%3s n:%3s> %-3s @ %-5s(%-4s)",
                    priceToString(itr->price_).c_str(), priceToString(itr->prev_entry_->price_).c_str(),
                    priceToString(itr->next_entry_->price_).c_str(),
                    priceToString(itr->price_).c_str(), qtyToString(qty).c_str(), std::to_string(num_orders).c_str());
            ss << buf;

            for (auto o_itr = itr->first_mkt_order_;; o_itr = o_itr->next_order_) {
                if (detailed) {
                    sprintf(buf, "[oid:%s q:%s p:%s n:%s] ",
                            orderIdToString(o_itr->order_id_).c_str(), qtyToString(o_itr->qty_).c_str(),
                            orderIdToString(o_itr->prev_order_ ? o_itr->prev_order_->order_id_ : OrderId_INVALID).c_str(),
                            orderIdToString(o_itr->next_order_ ? o_itr->next_order_->order_id_ : OrderId_INVALID).c_str());
                    ss << buf;
                }
                if (o_itr->next_order_ == itr->first_mkt_order_)
                    break;
            }

            ss << std::endl;

            if (sanity_check) {
                if ((side == Side::SELL && last_price >= itr->price_) || (side == Side::BUY && last_price <= itr->price_)) {
                    FATAL("Bids/Asks not sorted by ascending/descending prices last:" + priceToString(last_price) + " itr:" +
                            itr->toString());
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
}