#pragma once

#include "common/logger.hpp"
#include "common/mcast_socket.hpp"
#include "market_data/market_update.hpp"
#include "market_data/snapshot_synthesizer.hpp"

namespace Exchange {
class MarketDataPublisher {
private:
    MEMarketUpdateLFQueue* outgoing_md_updates_ = nullptr;
    size_t next_inc_seq_num_ = 1;
    PubMarketUpdateLFQueue snapshot_md_updates_;

    volatile bool running_ = false;

    Logger logger_;
    std::string time_str_;

    Common::MCastSocket incremental_updates_socket_;

    SnapshotSynthesizer* snapshot_synthesizer_;

    MEMarketUpdate market_update_;
    PubMarketUpdate pub_market_update_;

public:
    MarketDataPublisher(MEMarketUpdateLFQueue* outgoing_md_updates, const std::string &iface,
        const std::string &snapshot_ip, int snapshot_port, const std::string &incremental_ip, int incremental_port) 
        : outgoing_md_updates_(outgoing_md_updates), snapshot_md_updates_(ME_MAX_MARKET_UPDATES), 
        logger_("exchange_market_data_publisher.log"), incremental_updates_socket_(logger_) {
            ASSERT(incremental_updates_socket_.init(incremental_ip, iface, incremental_port, false) >= 0, "Unable to create incremental mcast socket. error:" + std::string(std::strerror(errno)));
            snapshot_synthesizer_ = new SnapshotSynthesizer(&snapshot_md_updates_, iface, snapshot_ip, snapshot_port);
        }

    ~MarketDataPublisher() {
        stop();

        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(5s);

        delete snapshot_synthesizer_;
        snapshot_synthesizer_ = nullptr;
    }

    void start() {
        running_ = true;
        ASSERT(createAndStartThread(-1, "Exchange/MarketDataPublisher", [this]() { run(); }) != nullptr, "Failed to start MarketDataPublisher thread");
        snapshot_synthesizer_->start();
    }

    void stop() {
        running_ = false;
        snapshot_synthesizer_->stop();
    }

    void run() {
        logger_.log("%:% %() %\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str_));
        while (running_) {
            while (outgoing_md_updates_->pop(market_update_)) {
                logger_.log("%:% %() % Sending seq:% %\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str_), next_inc_seq_num_,
                            market_update_.toString().c_str());

                incremental_updates_socket_.send(&next_inc_seq_num_, sizeof(next_inc_seq_num_));
                incremental_updates_socket_.send(&market_update_, sizeof(MEMarketUpdate));

                pub_market_update_ = {next_inc_seq_num_, std::move(market_update_)};
                snapshot_md_updates_.push(std::move(pub_market_update_));
                ++next_inc_seq_num_;
            }

            incremental_updates_socket_.sendAndRecv();
        }
    }

    MarketDataPublisher() = delete;
    MarketDataPublisher(const MarketDataPublisher&) = delete;
    MarketDataPublisher(const MarketDataPublisher&&) = delete;
    MarketDataPublisher& operator=(const MarketDataPublisher&) = delete;
    MarketDataPublisher& operator=(const MarketDataPublisher&&) = delete;
};
}