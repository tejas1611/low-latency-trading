#pragma once

#include "common/mcast_socket.hpp"
#include "common/logger.hpp"
#include "common/mem_pool.hpp"
#include "common/macros.hpp"
#include "market_data/market_update.hpp"
#include "matching_engine/me_order.hpp"

namespace Exchange
{
class SnapshotSynthesizer {
private:
    PubMarketUpdateLFQueue* snapshot_md_updates_ = nullptr;
    
    volatile bool running_ = false;

    std::array<std::array<MEMarketUpdate*, ME_MAX_ORDER_IDS>, ME_MAX_TICKERS> ticker_orders_;
    size_t last_inc_seq_num_ = 0;
    Nanos last_snapshot_time_ = 0;

    Logger logger_;
    std::string time_str_;

    Common::MCastSocket snapshot_updates_socket_;
    Common::MemPool<MEMarketUpdate> order_pool_;
    
    PubMarketUpdate market_update_;

public:
    SnapshotSynthesizer(PubMarketUpdateLFQueue* snapshot_md_updates, const std::string &iface, const std::string &snapshot_ip, int snapshot_port);
    ~SnapshotSynthesizer();

    void start();
    void stop();
    void run();
    void addToSnapshot(const PubMarketUpdate* market_update);
    void publishSnapshot();

    SnapshotSynthesizer() = delete;
    SnapshotSynthesizer(const SnapshotSynthesizer&) = delete;
    SnapshotSynthesizer(const SnapshotSynthesizer&&) = delete;
    SnapshotSynthesizer& operator=(const SnapshotSynthesizer&) = delete;
    SnapshotSynthesizer& operator=(const SnapshotSynthesizer&&) = delete;
};
}
