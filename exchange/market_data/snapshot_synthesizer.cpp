#include "market_data/snapshot_synthesizer.hpp"

namespace Exchange {
    SnapshotSynthesizer::SnapshotSynthesizer(PubMarketUpdateLFQueue* snapshot_md_updates, const std::string &iface, 
        const std::string &snapshot_ip, int snapshot_port) : snapshot_md_updates_(snapshot_md_updates), logger_("exchange_snapshot_synthesizer.log"),
        snapshot_socket_(logger_), order_pool_(ME_MAX_ORDER_IDS) {
            ASSERT(snapshot_socket_.init(snapshot_ip, iface, snapshot_port, /*is_listening*/ false) >= 0, "Unable to create snapshot mcast socket. error:" + std::string(std::strerror(errno)));
        for(auto& orders : ticker_orders_) {
            orders.fill(nullptr);
        }
    }

    SnapshotSynthesizer::~SnapshotSynthesizer() {
        stop();
    }
    
    void SnapshotSynthesizer::start() {
        running_ = true;

        ASSERT(createAndStartThread(-1, "Exchange/SnapshotSynthesizer", [this]() { run(); }) != nullptr, "Failed to start SnapshotSynthesizer thread");
    }
    
    void SnapshotSynthesizer::stop() {
        running_ = false;
    }

    void SnapshotSynthesizer::run() {
        logger_.log("%:% %() %\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_));
        while (running_) {
            while (snapshot_md_updates_->pop(market_update_)) {
                logger_.log("%:% %() % Processing %\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_),
                    market_update->toString().c_str());

                addToSnapshot(market_update_);
            }
        }

        if (getCurrentNanos() - last_snapshot_time_ > 60 * NANOS_TO_SECS) {
            last_snapshot_time_ = getCurrentNanos();
            publishSnapshot();
        }
    }

    void SnapshotSynthesizer::addToSnapshot(const PubMarketUpdate* market_update) {
        const auto& me_market_update = market_update->me_market_update_;
        auto *orders = &ticker_orders_.at(me_market_update.ticker_id_);
        switch (me_market_update.type_) {
        case MarketUpdateType::ADD:
            auto order = orders->at(me_market_update.order_id_);
            ASSERT(order == nullptr, "Received:" + me_market_update.toString() + " but order already exists:" + (order ? order->toString() : ""));
            orders->at(me_market_update.order_id_) = order_pool_.allocate(me_market_update);
        break;
        case MarketUpdateType::MODIFY:
            auto order = orders->at(me_market_update.order_id_);
            ASSERT(order != nullptr, "Received:" + me_market_update.toString() + " but order does not exist.");
            ASSERT(order->order_id_ == me_market_update.order_id_, "Expecting existing order to match new one.");
            ASSERT(order->side_ == me_market_update.side_, "Expecting existing order to match new one.");

            order->qty_ = me_market_update.qty_;
            order->price_ = me_market_update.price_;
        break;
        case MarketUpdateType::CANCEL:
            auto order = orders->at(me_market_update.order_id_);
            ASSERT(order != nullptr, "Received:" + me_market_update.toString() + " but order does not exist.");
            ASSERT(order->order_id_ == me_market_update.order_id_, "Expecting existing order to match new one.");
            ASSERT(order->side_ == me_market_update.side_, "Expecting existing order to match new one.");

            order_pool_.deallocate(order);
            orders->at(me_market_update.order_id_) = nullptr;
        break;
        case MarketUpdateType::SNAPSHOT_START:
        case MarketUpdateType::CLEAR:
        case MarketUpdateType::SNAPSHOT_END:
        case MarketUpdateType::TRADE:
        case MarketUpdateType::INVALID:
        break;
        }

        ASSERT(market_update->seq_num_ == last_inc_seq_num_ + 1, "Expected incremental seq_nums to increase.");
        last_inc_seq_num_ = market_update->seq_num_;
    }

    void SnapshotSynthesizer::publishSnapshot() {
        size_t snapshot_size = 0;

        const PubMarketUpdate start_market_update{snapshot_size++, {MarketUpdateType::SNAPSHOT_START, last_inc_seq_num_}};
        logger_.log("%:% %() % %\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_), start_market_update.toString());
        snapshot_socket_.send(&start_market_update, sizeof(PubMarketUpdate));

        for (size_t ticker_id = 0; ticker_id < ticker_orders_.size(); ++ticker_id) {
            const auto &orders = ticker_orders_.at(ticker_id);

            MEMarketUpdate me_market_update;
            me_market_update.type_ = MarketUpdateType::CLEAR;
            me_market_update.ticker_id_ = ticker_id;

            const PubMarketUpdate clear_market_update{snapshot_size++, me_market_update};
            logger_.log("%:% %() % %\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_), clear_market_update.toString());
            snapshot_socket_.send(&clear_market_update, sizeof(PubMarketUpdate));

            for (const auto order: orders) {
                if (order) {
                    const PubMarketUpdate market_update{snapshot_size++, *order};
                    logger_.log("%:% %() % %\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_), market_update.toString());
                    snapshot_socket_.send(&market_update, sizeof(PubMarketUpdate));
                    snapshot_socket_.sendAndRecv();
                }
            }
        }

        const PubMarketUpdate end_market_update{snapshot_size++, {MarketUpdateType::SNAPSHOT_END, last_inc_seq_num_}};
        logger_.log("%:% %() % %\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_), end_market_update.toString());
        snapshot_socket_.send(&end_market_update, sizeof(PubMarketUpdate));
        snapshot_socket_.sendAndRecv();

        logger_.log("%:% %() % Published snapshot of % orders.\n", __FILE__, __LINE__, __FUNCTION__, getCurrentTimeStr(&time_str_), snapshot_size - 1);
    }
}