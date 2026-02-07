#pragma once

#include <functional>
#include <future>
#include <mutex>
#include <set>
#include <string>
#include <vector>

#include <boost/asio.hpp>
#include <WssWorker.h>

namespace connectors {

struct BBO {
    std::string market;
    double bestBid = 0.0;
    double bestBidSize = 0.0;
    double bestAsk = 0.0;
    double bestAskSize = 0.0;
    double lastPrice = 0.0;
};

struct OrderBookEntry {
    double price = 0.0;
    double size = 0.0;
};

struct OrderBook {
    std::string market;
    int64_t nonce = 0;
    std::vector<OrderBookEntry> bids;
    std::vector<OrderBookEntry> asks;
};

struct PublicTrade {
    std::string market;
    std::string id;
    double price = 0.0;
    double amount = 0.0;
    std::string side;       // "buy" or "sell"
    int64_t timestamp = 0;  // UTC milliseconds
};

enum class ClientState {
    Disconnected,
    Connecting,
    Connected
};

class BitvavoClient {
public:
    struct Callbacks {
        std::function<void(const BBO&)> HandleBBO;
        std::function<void(const OrderBook&)> HandleOrderBook;
        std::function<void(const PublicTrade&)> HandlePublicTrade;
        std::function<void(const std::string&)> HandleError;
        std::function<void(bool)> HandleConnection;
    };

    BitvavoClient(boost::asio::io_context& io_context, Callbacks callbacks);
    ~BitvavoClient();

    BitvavoClient(const BitvavoClient&) = delete;
    BitvavoClient& operator=(const BitvavoClient&) = delete;

    std::future<bool> Connect();
    void Disconnect();

    std::future<bool> SubscribeTicker(std::vector<std::string> markets);
    std::future<bool> UnsubscribeTicker(std::vector<std::string> markets);

    ClientState GetState() const { return state_; }

private:
    void OnWsMessage(const std::string& message);
    void OnWsError(const std::string& error);
    void OnWsConnection(bool connected);

    void HandleTickerEvent(const std::string& message);

    static std::string BuildSubscribeJson(const std::string& action,
                                          const std::string& channel,
                                          const std::vector<std::string>& markets);

    boost::asio::io_context& io_context_;
    std::unique_ptr<WssWorker> worker_;
    ClientState state_ = ClientState::Disconnected;

    Callbacks callbacks_;

    std::mutex subscription_mutex_;
    std::set<std::string> subscribed_markets_;

    std::promise<bool> subscribe_promise_;
    std::promise<bool> unsubscribe_promise_;
    bool subscribe_pending_ = false;
    bool unsubscribe_pending_ = false;
};

} // namespace connectors
