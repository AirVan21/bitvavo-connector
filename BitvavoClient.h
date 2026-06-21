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

/// @brief WebSocket client for the Bitvavo exchange.
///
/// Implements MarketDataConnector, accepting canonical instrument_ids and
/// resolving them to Bitvavo venue symbols via InstrumentClient before sending
/// subscribe/unsubscribe payloads. All callbacks execute on the io_context thread.
struct BitvavoClient : MarketDataConnector {
    /// @brief User-supplied event handlers. Any handler left null is silently skipped.
    struct Callbacks {
        std::function<void(const BBO&)> handle_bbo_;              ///< Fired on each ticker event.
        std::function<void(const OrderBook&)> handle_order_book_; ///< Reserved, not yet implemented.
        std::function<void(const PublicTrade&)> handle_public_trade_; ///< Fired on each trade event.
        std::function<void(const std::string&)> handle_error_;    ///< Fired on parse or lookup errors.
        std::function<void(bool)> handle_connection_;             ///< @p true = connected, @p false = disconnected.
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
    /// Translates instrument_ids to Bitvavo venue symbols via InstrumentClient::ResolveListing.
    /// @return Resolved symbols in the same order as @p instrument_ids, or an empty vector if
    ///         any id has no listing (handle_error_ is fired for the offending id).
    std::vector<std::string> ResolveVenueSymbols(const std::vector<int64_t>& instrument_ids);

    void OnWsMessage(const std::string& message);
    void OnWsError(const std::string& error);
    void OnWsConnection(bool connected);

    void HandleTickerEvent(const std::string& message);
    void HandleTradeEvent(const std::string& message);

    /// @brief Sends a Bitvavo subscribe/unsubscribe JSON payload.
    ///
    /// Arms @p pending and resets @p promise so that the next matching ACK event
    /// (via ResolveSubscription) resolves the returned future.
    /// @return Future that resolves to @p true on ACK, @p false if not connected.
    std::future<bool> SendSubscription(const std::string& action,
                                        const std::string& channel,
                                        std::vector<std::string> markets,
                                        bool& pending,
                                        std::promise<bool>& promise);

    /// @brief Resolves the pending promise when a "subscribed"/"unsubscribed" ACK arrives.
    /// No-op if @p pending is false (no in-flight request for this slot).
    void ResolveSubscription(bool& pending, std::promise<bool>& promise);

    static std::string BuildSubscribeJson(const std::string& action,
                                          const std::string& channel,
                                          const std::vector<std::string>& markets);

    boost::asio::io_context& io_context_;
    InstrumentClient& instrument_client_;
    std::unique_ptr<WssWorker> worker_;
    ClientState state_ = ClientState::Disconnected;

    Callbacks callbacks_;

    /// Each subscribe/unsubscribe call resets its promise and sets pending=true before sending.
    /// The next matching ACK from Bitvavo resolves the promise via ResolveSubscription.
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
