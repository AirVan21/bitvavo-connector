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

enum class ClientState {
    Disconnected,
    Connecting,
    Connected
};

using BBOCallback = std::function<void(const BBO&)>;
using ErrorCallback = std::function<void(const std::string&)>;
using ConnectionCallback = std::function<void(bool)>;

class BitvavoClient {
public:
    explicit BitvavoClient(boost::asio::io_context& io_context);
    ~BitvavoClient();

    BitvavoClient(const BitvavoClient&) = delete;
    BitvavoClient& operator=(const BitvavoClient&) = delete;

    std::future<bool> Connect();
    void Disconnect();

    std::future<bool> SubscribeTicker(std::vector<std::string> markets);
    std::future<bool> UnsubscribeTicker(std::vector<std::string> markets);

    void SetBBOCallback(BBOCallback callback);
    void SetErrorCallback(ErrorCallback callback);
    void SetConnectionCallback(ConnectionCallback callback);

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

    BBOCallback bbo_callback_;
    ErrorCallback error_callback_;
    ConnectionCallback connection_callback_;

    std::mutex subscription_mutex_;
    std::set<std::string> subscribed_markets_;

    std::promise<bool> subscribe_promise_;
    std::promise<bool> unsubscribe_promise_;
    bool subscribe_pending_ = false;
    bool unsubscribe_pending_ = false;
};

} // namespace connectors
