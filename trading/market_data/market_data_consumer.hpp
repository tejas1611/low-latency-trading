#pragma once

#include <map>

#include "common/logger.hpp"
#include "common/mcast_socket.hpp"
#include "market_data/market_update.hpp"

namespace Trading {
    class MarketDataConsumer {
    private:
        size_t next_exp_inc_seq_num_ = 1;
        Exchange::MEMarketUpdateLFQueue* incoming_md_updates_ = nullptr;
        
        volatile bool running_ = false;
        
        Logger logger_;
        std::string time_str_;
        
        Common::MCastSocket incremental_updates_socket_;
        Common::MCastSocket snapshot_updates_socket_;
        
        bool in_recovery_ = false;
        const std::string iface_;
        const std::string snapshot_ip_;
        const int snapshot_port_;
        
        typedef std::map<size_t, Exchange::MEMarketUpdate> QueuedMarketUpdates;
        QueuedMarketUpdates snapshot_queued_msgs_; 
        QueuedMarketUpdates incremental_queued_msgs_;
    
    public:
        MarketDataConsumer(Common::ClientId client_id, Exchange::MEMarketUpdateLFQueue* market_updates, const std::string& iface,
            const std::string& snapshot_ip, int snapshot_port,
            const std::string& incremental_ip, int incremental_port);

        ~MarketDataConsumer();

        void start();
        void stop();

        MarketDataConsumer() = delete;
        MarketDataConsumer(MarketDataConsumer&) = delete;
        MarketDataConsumer(MarketDataConsumer&&) = delete;
        MarketDataConsumer& operator=(MarketDataConsumer&) = delete;
        MarketDataConsumer& operator=(MarketDataConsumer&&) = delete;

    private:
        void run() noexcept;
        void recvCallback(McastSocket* socket) noexcept;

        void startSnapshotSync();
        void checkSnapshotSync();
        void queueMessage(bool is_snapshot, const Exchange::PubMarketUpdate* request);
    };
}