#pragma once

#include "order_server/client_request.hpp"
#include "matching_engine/me_orderbook.hpp"

#include "common/macros.hpp"

namespace Exchange{
    class MatchingEngine final {
    private:
        OrderBookHashMap ticker_order_book_;

        ClientRequestLFQueue* incoming_requests_ = nullptr;
        ClientResponseLFQueue* outgoing_responses_ = nullptr;
        MEMarketUpdateLFQueue* outgoing_md_updates_ = nullptr;

        volatile bool running_ = false;
        
        std::string time_str_;
        Logger logger_;

        MEClientRequest me_client_request;

    public:
        MatchingEngine(ClientRequestLFQueue *client_requests, ClientResponseLFQueue *client_responses, MEMarketUpdateLFQueue *market_updates) :
        incoming_requests_(client_requests), outgoing_responses_(client_responses), outgoing_md_updates_(market_updates), logger_("exchange_matching_engine.log") {
            for(auto i = 0uL; i < ticker_order_book_.size(); ++i) {
                ticker_order_book_[i] = new MEOrderBook(i, this, &logger_);
            }
        }

        ~MatchingEngine() {
            running_ = false;

            using namespace std::literals::chrono_literals;
            std::this_thread::sleep_for(1s);    

            incoming_requests_ = nullptr;
            outgoing_responses_ = nullptr;
            outgoing_md_updates_ = nullptr;

            for (auto& orderbook : ticker_order_book_) {
                delete orderbook;
                orderbook = nullptr;
            }
        }

        void start() {
            running_ = true;
            ASSERT(Common::createAndStartThread(-1, "Exchange/MatchingEngine", [this]() { run(); }) != nullptr, "Failed to start MatchingEngine thread.");
        }

        void stop() {
            running_ = false;
        }

        void sendClientResponse(const MEClientResponse& client_response) {
            logger_.log("%:% %() % Sending %\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str_), client_response.toString());
            outgoing_responses_->push(client_response);
        }

        void sendMarketUpdate(const MEMarketUpdate& market_update) {
            logger_.log("%:% %() % Sending %\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str_), market_update.toString());
            outgoing_md_updates_->push(market_update);
        }

        MatchingEngine() = delete;
        MatchingEngine(MatchingEngine&) = delete;
        MatchingEngine(MatchingEngine&&) = delete;
        MatchingEngine& operator=(MatchingEngine&) = delete;
        MatchingEngine& operator=(MatchingEngine&&) = delete;

    private:
        void processClientRequest(const MEClientRequest& client_request) noexcept {
            MEOrderBook* order_book = ticker_order_book_[client_request.ticker_id_];
            switch (client_request.type_) {
                case ClientRequestType::NEW:
                    order_book->add(client_request.client_id_, client_request.client_order_id_, client_request.side_, 
                            client_request.price_, client_request.qty_);
                break;
                case ClientRequestType::CANCEL:
                    order_book->cancel(client_request.client_id_, client_request.client_order_id_);
                break;
                default:
                    FATAL("Received invalid client-request-type:" + clientRequestTypeToString(client_request.type_));
            }
        }
        
        void run() noexcept {
            while (running_) {
                if (incoming_requests_->pop(me_client_request)) [[likely]] {
                    logger_.log("%:% %() % Processing %\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&time_str_),
                      me_client_request.toString());
                    processClientRequest(me_client_request);
                }
            }
        }
    };
}