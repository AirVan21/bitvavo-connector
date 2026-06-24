#include "BitvavoClient.h"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace connectors {

BitvavoClient::BitvavoClient(boost::asio::io_context& io_context,
                             InstrumentClient& instrument_client,
                             Callbacks callbacks)
    : io_context_(io_context),
      instrument_client_(instrument_client),
      callbacks_(std::move(callbacks)) {}

BitvavoClient::~BitvavoClient() {
    Disconnect();
}

std::future<bool> BitvavoClient::Connect() {
    if (state_ != ClientState::Disconnected) {
        std::promise<bool> promise;
        promise.set_value(false);
        return promise.get_future();
    }

    state_ = ClientState::Connecting;

    ::connectors::Callbacks ws_callbacks;
    ws_callbacks.on_message = [this](const std::string& msg) { OnWsMessage(msg); };
    ws_callbacks.on_error = [this](const std::string& err) { OnWsError(err); };
    ws_callbacks.on_connection = [this](bool connected) { OnWsConnection(connected); };

    worker_ = std::make_unique<WssWorker>(io_context_, std::move(ws_callbacks));

    ConnectionSettings settings{"ws.bitvavo.com", "443", "/v2/"};
    return worker_->Connect(settings);
}

void BitvavoClient::Disconnect() {
    if (!worker_) return;

    worker_->StopListening();
    worker_->Disconnect();
    worker_.reset();
    state_ = ClientState::Disconnected;

    if (callbacks_.handle_connection_) {
        callbacks_.handle_connection_(false);
    }
}

std::vector<std::string> BitvavoClient::ResolveVenueSymbols(
    const std::vector<int64_t>& instrument_ids) {
    std::vector<std::string> venue_symbols;
    venue_symbols.reserve(instrument_ids.size());

    for (const auto instrument_id : instrument_ids) {
        auto listing = instrument_client_.ResolveListing(instrument_id, Venue());
        if (!listing) {
            if (callbacks_.handle_error_) {
                callbacks_.handle_error_("No bitvavo listing for instrument_id=" +
                                         std::to_string(instrument_id));
            }
            return {};
        }
        venue_symbols.push_back(listing->venue_symbol_);
    }

    return venue_symbols;
}

std::future<bool> BitvavoClient::SubscribeBBO(std::vector<int64_t> instrument_ids) {
    auto venue_symbols = ResolveVenueSymbols(instrument_ids);
    if (venue_symbols.empty()) {
        std::promise<bool> promise;
        promise.set_value(false);
        return promise.get_future();
    }
    return SendSubscription("subscribe", "ticker", std::move(venue_symbols),
                            subscribe_bbo_pending_, subscribe_bbo_promise_);
}

std::future<bool> BitvavoClient::UnsubscribeBBO(std::vector<int64_t> instrument_ids) {
    auto venue_symbols = ResolveVenueSymbols(instrument_ids);
    if (venue_symbols.empty()) {
        std::promise<bool> promise;
        promise.set_value(false);
        return promise.get_future();
    }
    return SendSubscription("unsubscribe", "ticker", std::move(venue_symbols),
                            unsubscribe_bbo_pending_, unsubscribe_bbo_promise_);
}

std::future<bool> BitvavoClient::SubscribeTrades(std::vector<int64_t> instrument_ids) {
    auto venue_symbols = ResolveVenueSymbols(instrument_ids);
    if (venue_symbols.empty()) {
        std::promise<bool> promise;
        promise.set_value(false);
        return promise.get_future();
    }
    return SendSubscription("subscribe", "trades", std::move(venue_symbols),
                            subscribe_trades_pending_, subscribe_trades_promise_);
}

std::future<bool> BitvavoClient::UnsubscribeTrades(std::vector<int64_t> instrument_ids) {
    auto venue_symbols = ResolveVenueSymbols(instrument_ids);
    if (venue_symbols.empty()) {
        std::promise<bool> promise;
        promise.set_value(false);
        return promise.get_future();
    }
    return SendSubscription("unsubscribe", "trades", std::move(venue_symbols),
                            unsubscribe_trades_pending_, unsubscribe_trades_promise_);
}

std::future<bool> BitvavoClient::SendSubscription(const std::string& action,
                                                   const std::string& channel,
                                                   std::vector<std::string> markets,
                                                   bool& pending,
                                                   std::promise<bool>& promise) {
    if (state_ != ClientState::Connected) {
        std::promise<bool> result;
        result.set_value(false);
        return result.get_future();
    }

    auto json = BuildSubscribeJson(action, channel, markets);

    promise = std::promise<bool>();
    pending = true;
    auto future = promise.get_future();

    worker_->Send(json);

    return future;
}

void BitvavoClient::OnWsMessage(const std::string& message) {
    rapidjson::Document doc;
    doc.Parse(message.c_str());

    if (doc.HasParseError()) {
        if (callbacks_.handle_error_) {
            callbacks_.handle_error_("Failed to parse JSON: " + message);
        }
        return;
    }

    if (!doc.IsObject()) {
        if (callbacks_.handle_error_) {
            callbacks_.handle_error_("JSON is not an object: " + message);
        }
        return;
    }

    if (doc.HasMember("event") && doc["event"].IsString()) {
        std::string event = doc["event"].GetString();

        if (event == "ticker") {
            HandleTickerEvent(message);
        } else if (event == "trade") {
            HandleTradeEvent(message);
        } else if (event == "subscribed") {
            ResolveSubscription(subscribe_bbo_pending_, subscribe_bbo_promise_);
            ResolveSubscription(subscribe_trades_pending_, subscribe_trades_promise_);
        } else if (event == "unsubscribed") {
            ResolveSubscription(unsubscribe_bbo_pending_, unsubscribe_bbo_promise_);
            ResolveSubscription(unsubscribe_trades_pending_, unsubscribe_trades_promise_);
        }
    }
}

void BitvavoClient::ResolveSubscription(bool& pending, std::promise<bool>& promise) {
    if (pending) {
        pending = false;
        promise.set_value(true);
    }
}

void BitvavoClient::OnWsError(const std::string& error) {
    if (callbacks_.handle_error_) {
        callbacks_.handle_error_(error);
    }
}

void BitvavoClient::OnWsConnection(bool connected) {
    if (connected) {
        state_ = ClientState::Connected;
        worker_->StartListening();
    } else {
        state_ = ClientState::Disconnected;
    }

    if (callbacks_.handle_connection_) {
        callbacks_.handle_connection_(connected);
    }
}

void BitvavoClient::HandleTickerEvent(const std::string& message) {
    rapidjson::Document doc;
    doc.Parse(message.c_str());

    if (doc.HasParseError() || !doc.IsObject()) {
        if (callbacks_.handle_error_) {
            callbacks_.handle_error_("Invalid ticker JSON: " + message);
        }
        return;
    }

    if (!doc.HasMember("market") || !doc["market"].IsString()) {
        if (callbacks_.handle_error_) {
            callbacks_.handle_error_("Missing or invalid 'market' field in ticker: " + message);
        }
        return;
    }

    try {
        BBO bbo;
        const std::string market = doc["market"].GetString();
        if (auto instrument_id = instrument_client_.ResolveInstrumentId(Venue(), market)) {
            bbo.instrument_id_ = *instrument_id;
        } else if (callbacks_.handle_error_) {
            callbacks_.handle_error_("No cached instrument_id for ticker market=" + market);
        }

        if (doc.HasMember("bestBid") && doc["bestBid"].IsString()) {
            bbo.best_bid_ = std::stod(doc["bestBid"].GetString());
        }
        if (doc.HasMember("bestBidSize") && doc["bestBidSize"].IsString()) {
            bbo.best_bid_size_ = std::stod(doc["bestBidSize"].GetString());
        }
        if (doc.HasMember("bestAsk") && doc["bestAsk"].IsString()) {
            bbo.best_ask_ = std::stod(doc["bestAsk"].GetString());
        }
        if (doc.HasMember("bestAskSize") && doc["bestAskSize"].IsString()) {
            bbo.best_ask_size_ = std::stod(doc["bestAskSize"].GetString());
        }

        if (callbacks_.handle_bbo_) {
            callbacks_.handle_bbo_(bbo);
        }
    } catch (const std::exception& e) {
        if (callbacks_.handle_error_) {
            callbacks_.handle_error_(std::string("Failed to parse ticker values: ") + e.what());
        }
    }
}

void BitvavoClient::HandleTradeEvent(const std::string& message) {
    rapidjson::Document doc;
    doc.Parse(message.c_str());

    if (doc.HasParseError() || !doc.IsObject()) {
        if (callbacks_.handle_error_) {
            callbacks_.handle_error_("Invalid trade JSON: " + message);
        }
        return;
    }

    const char* required_string_fields[] = {"market", "id", "price", "amount", "side"};
    for (const auto* field : required_string_fields) {
        if (!doc.HasMember(field) || !doc[field].IsString()) {
            if (callbacks_.handle_error_) {
                callbacks_.handle_error_(std::string("Missing or invalid field '") + field +
                                         "' in trade: " + message);
            }
            return;
        }
    }

    if (!doc.HasMember("timestamp") || !doc["timestamp"].IsInt64()) {
        if (callbacks_.handle_error_) {
            callbacks_.handle_error_("Missing or invalid field 'timestamp' in trade: " + message);
        }
        return;
    }

    try {
        PublicTrade trade;
        const std::string market = doc["market"].GetString();
        if (auto instrument_id = instrument_client_.ResolveInstrumentId(Venue(), market)) {
            trade.instrument_id_ = *instrument_id;
        } else if (callbacks_.handle_error_) {
            callbacks_.handle_error_("No cached instrument_id for trade market=" + market);
        }

        trade.trade_id_ = doc["id"].GetString();
        trade.price_ = std::stod(doc["price"].GetString());
        trade.amount_ = std::stod(doc["amount"].GetString());
        trade.side_ = doc["side"].GetString();
        trade.timestamp_ = doc["timestamp"].GetInt64();

        if (callbacks_.handle_public_trade_) {
            callbacks_.handle_public_trade_(trade);
        }
    } catch (const std::exception& e) {
        if (callbacks_.handle_error_) {
            callbacks_.handle_error_(std::string("Failed to parse trade values: ") + e.what());
        }
    }
}

std::string BitvavoClient::BuildSubscribeJson(const std::string& action,
                                               const std::string& channel,
                                               const std::vector<std::string>& markets) {
    rapidjson::Document doc;
    doc.SetObject();
    auto& alloc = doc.GetAllocator();

    doc.AddMember("action", rapidjson::Value(action.c_str(), alloc), alloc);

    rapidjson::Value channels(rapidjson::kArrayType);
    rapidjson::Value chan(rapidjson::kObjectType);
    chan.AddMember("name", rapidjson::Value(channel.c_str(), alloc), alloc);

    rapidjson::Value markets_array(rapidjson::kArrayType);
    for (const auto& market : markets) {
        markets_array.PushBack(rapidjson::Value(market.c_str(), alloc), alloc);
    }
    chan.AddMember("markets", markets_array, alloc);
    channels.PushBack(chan, alloc);

    doc.AddMember("channels", channels, alloc);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    return buffer.GetString();
}

} // namespace connectors
