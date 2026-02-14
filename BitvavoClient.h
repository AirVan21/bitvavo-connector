#pragma once

#include <functional>
#include <future>
#include <optional>
#include <string>
#include <vector>

#include <boost/asio.hpp>
#include <WssWorker.h>

namespace connectors {

struct BBO {
    std::string market_;
    std::optional<double> best_bid_;
    std::optional<double> best_bid_size_;
    std::optional<double> best_ask_;
    std::optional<double> best_ask_size_;
};

struct OrderBookEntry {
    double price_ = 0.0;
    double size_ = 0.0;
};

struct OrderBook {
    std::string market_;
    int64_t nonce_ = 0;
    std::vector<OrderBookEntry> bids_;
    std::vector<OrderBookEntry> asks_;
};

struct PublicTrade {
    std::string market_;
    std::string id_;
    double price_ = 0.0;
    double amount_ = 0.0;
    std::string side_;       // "buy" or "sell"
    int64_t timestamp_ = 0;  // UTC milliseconds
};

enum class ClientState {
    Disconnected,
    Connecting,
    Connected
};

struct BitvavoClient {
    struct Callbacks {
        std::function<void(const BBO&)> handle_bbo_;
        std::function<void(const OrderBook&)> handle_order_book_;
        std::function<void(const PublicTrade&)> handle_public_trade_;
        std::function<void(const std::string&)> handle_error_;
        std::function<void(bool)> handle_connection_;
    };

    BitvavoClient(boost::asio::io_context& io_context, Callbacks callbacks);
    ~BitvavoClient();

    BitvavoClient(const BitvavoClient&) = delete;
    BitvavoClient& operator=(const BitvavoClient&) = delete;

    std::future<bool> Connect();
    void Disconnect();

    std::future<bool> SubscribeTicker(std::vector<std::string> markets);
    std::future<bool> UnsubscribeTicker(std::vector<std::string> markets);

    std::future<bool> SubscribeTrades(std::vector<std::string> markets);
    std::future<bool> UnsubscribeTrades(std::vector<std::string> markets);

    ClientState GetState() const { return state_; }

private:
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
