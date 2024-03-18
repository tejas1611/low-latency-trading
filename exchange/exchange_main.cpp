#include <csignal>

#include "matching_engine/matching_engine.hpp"
#include "order_server/order_server.hpp"
#include "market_data/market_data_publisher.hpp"

Common::Logger* logger = nullptr;
Exchange::MatchingEngine* matching_engine = nullptr;
Exchange::OrderServer* order_server = nullptr;
Exchange::MarketDataPublisher* market_data_publisher = nullptr;

void signal_handler(int) {
    using namespace std::literals::chrono_literals;
    std::this_thread::sleep_for(10s);

    delete logger; logger = nullptr;
    delete matching_engine; matching_engine = nullptr;
    delete order_server; matching_engine = nullptr;
    delete market_data_publisher; matching_engine = nullptr;

    std::this_thread::sleep_for(10s);

    exit(EXIT_SUCCESS);
}

int main() {
    logger = new Common::Logger("exchange_main.log");

    std::signal(SIGINT, signal_handler);

    const int sleep_time = 100 * 1000;

    Exchange::ClientRequestLFQueue client_requests(ME_MAX_CLIENT_UPDATES);
    Exchange::ClientResponseLFQueue client_responses(ME_MAX_CLIENT_UPDATES);
    Exchange::MEMarketUpdateLFQueue market_updates(ME_MAX_MARKET_UPDATES);

    std::string time_str;

    logger->log("%:% %() % Starting Matching Engine...\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str));
    matching_engine = new Exchange::MatchingEngine(&client_requests, &client_responses, &market_updates);
    matching_engine->start();

    const std::string mkt_pub_iface = "lo";
    const std::string snap_pub_ip = "233.252.14.1", inc_pub_ip = "233.252.14.3";
    const int snap_pub_port = 20000, inc_pub_port = 20001;

    logger->log("%:% %() % Starting Market Data Publisher...\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str));
    market_data_publisher = new Exchange::MarketDataPublisher(&market_updates, mkt_pub_iface, snap_pub_ip, snap_pub_port, inc_pub_ip, inc_pub_port);
    market_data_publisher->start();

    const std::string order_gw_iface = "lo";
    const int order_gw_port = 12345;

    logger->log("%:% %() % Starting Order Server...\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str));
    order_server = new Exchange::OrderServer(order_gw_iface, order_gw_port, &client_responses, &client_requests);
    order_server->start();

    while (true) {
        logger->log("%:% %() % Sleeping for a few milliseconds..\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str));
        usleep(sleep_time * 1000);
    }
}