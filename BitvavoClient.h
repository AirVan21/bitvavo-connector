#pragma once

#include <functional>
#include <future>
#include <memory>
#include <string>
#include <vector>

#include <boost/asio.hpp>
#include <WssWorker.h>

#include "connectors/BBO.h"
#include "connectors/InstrumentClient.h"
#include "connectors/MarketDataConnector.h"

namespace connectors {

enum class ClientState {
    Disconnected,
    Connecting,
    Connected
};

struct BitvavoClient : MarketDataConnector {
    struct Callbacks {
        std::function<void(const BBO&)> handle_bbo_;
        std::function<void(const OrderBook&)> handle_order_book_;
        std::function<void(const PublicTrade&)> handle_public_trade_;
        std::function<void(const std::string&)> handle_error_;
        std::function<void(bool)> handle_connection_;
    };

    BitvavoClient(boost::asio::io_context& io_context,
                  InstrumentClient& instrument_client,
                  Callbacks callbacks);
    ~BitvavoClient();

    BitvavoClient(const BitvavoClient&) = delete;
    BitvavoClient& operator=(const BitvavoClient&) = delete;

    std::future<bool> Connect() override;
    void Disconnect() override;

    std::future<bool> SubscribeBBO(std::vector<int64_t> instrument_ids) override;
    std::future<bool> SubscribeTrades(std::vector<int64_t> instrument_ids) override;
    std::string Venue() const override { return "bitvavo"; }

    std::future<bool> UnsubscribeBBO(std::vector<int64_t> instrument_ids);
    std::future<bool> UnsubscribeTrades(std::vector<int64_t> instrument_ids);

    ClientState GetState() const { return state_; }

private:
    std::vector<std::string> ResolveVenueSymbols(const std::vector<int64_t>& instrument_ids);

    void OnWsMessage(const std::string& message);
    void OnWsError(const std::string& error);
    void OnWsConnection(bool connected);

    void HandleTickerEvent(const std::string& message);
    void HandleTradeEvent(const std::string& message);
    std::future<bool> SendSubscription(const std::string& action,
                                        const std::string& channel,
                                        std::vector<std::string> markets,
                                        bool& pending,
                                        std::promise<bool>& promise);
    void ResolveSubscription(bool& pending, std::promise<bool>& promise);

    static std::string BuildSubscribeJson(const std::string& action,
                                          const std::string& channel,
                                          const std::vector<std::string>& markets);

    boost::asio::io_context& io_context_;
    InstrumentClient& instrument_client_;
    std::unique_ptr<WssWorker> worker_;
    ClientState state_ = ClientState::Disconnected;

    Callbacks callbacks_;

    std::promise<bool> subscribe_bbo_promise_;
    std::promise<bool> unsubscribe_bbo_promise_;
    bool subscribe_bbo_pending_ = false;
    bool unsubscribe_bbo_pending_ = false;

    std::promise<bool> subscribe_trades_promise_;
    std::promise<bool> unsubscribe_trades_promise_;
    bool subscribe_trades_pending_ = false;
    bool unsubscribe_trades_pending_ = false;
};

} // namespace connectors
