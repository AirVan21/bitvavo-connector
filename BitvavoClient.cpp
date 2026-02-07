#include "BitvavoClient.h"

#include <iostream>
#include <stdexcept>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

namespace connectors {

BitvavoClient::BitvavoClient(boost::asio::io_context& io_context, Callbacks callbacks)
    : io_context_(io_context), callbacks_(std::move(callbacks)) {
}

BitvavoClient::~BitvavoClient() {
    Disconnect();
}

std::future<bool> BitvavoClient::Connect() {
    if (state_ != ClientState::Disconnected) {
        std::promise<bool> p;
        p.set_value(false);
        return p.get_future();
    }

    state_ = ClientState::Connecting;

    ::connectors::Callbacks ws_callbacks;
    ws_callbacks.on_message = [this](const std::string& msg) { OnWsMessage(msg); };
    ws_callbacks.on_error = [this](const std::string& err) { OnWsError(err); };
    ws_callbacks.on_connection = [this](bool connected) { OnWsConnection(connected); };

    worker_ = std::make_unique<WssWorker>(io_context_, std::move(ws_callbacks));

    ConnectionSettings settings{"ws.bitvavo.com", "443", "/v2/"};
    auto future = worker_->Connect(settings);

    return future;
}

void BitvavoClient::Disconnect() {
    if (!worker_) return;

    worker_->StopListening();
    worker_->Disconnect();
    worker_.reset();
    state_ = ClientState::Disconnected;

    if (callbacks_.HandleConnection) {
        callbacks_.HandleConnection(false);
    }
}

std::future<bool> BitvavoClient::SubscribeTicker(std::vector<std::string> markets) {
    if (state_ != ClientState::Connected) {
        std::promise<bool> p;
        p.set_value(false);
        return p.get_future();
    }

    auto json = BuildSubscribeJson("subscribe", "ticker", markets);

    subscribe_promise_ = std::promise<bool>();
    subscribe_pending_ = true;
    auto future = subscribe_promise_.get_future();

    auto send_future = worker_->Send(json);
    // We don't block on send_future here; the ack from the server resolves subscribe_promise_

    return future;
}

std::future<bool> BitvavoClient::UnsubscribeTicker(std::vector<std::string> markets) {
    if (state_ != ClientState::Connected) {
        std::promise<bool> p;
        p.set_value(false);
        return p.get_future();
    }

    auto json = BuildSubscribeJson("unsubscribe", "ticker", markets);

    unsubscribe_promise_ = std::promise<bool>();
    unsubscribe_pending_ = true;
    auto future = unsubscribe_promise_.get_future();

    worker_->Send(json);

    return future;
}

std::future<bool> BitvavoClient::SubscribeTrades(std::vector<std::string> markets) {
    if (state_ != ClientState::Connected) {
        std::promise<bool> p;
        p.set_value(false);
        return p.get_future();
    }

    auto json = BuildSubscribeJson("subscribe", "trades", markets);

    subscribe_trades_promise_ = std::promise<bool>();
    subscribe_trades_pending_ = true;
    auto future = subscribe_trades_promise_.get_future();

    auto send_future = worker_->Send(json);
    // We don't block on send_future here; the ack from the server resolves subscribe_trades_promise_

    return future;
}

std::future<bool> BitvavoClient::UnsubscribeTrades(std::vector<std::string> markets) {
    if (state_ != ClientState::Connected) {
        std::promise<bool> p;
        p.set_value(false);
        return p.get_future();
    }

    auto json = BuildSubscribeJson("unsubscribe", "trades", markets);

    unsubscribe_trades_promise_ = std::promise<bool>();
    unsubscribe_trades_pending_ = true;
    auto future = unsubscribe_trades_promise_.get_future();

    worker_->Send(json);

    return future;
}

void BitvavoClient::OnWsMessage(const std::string& message) {
    rapidjson::Document doc;
    doc.Parse(message.c_str());

    if (doc.HasParseError()) {
        if (callbacks_.HandleError) {
            callbacks_.HandleError("Failed to parse JSON: " + message);
        }
        return;
    }

    if (!doc.IsObject()) {
        if (callbacks_.HandleError) {
            callbacks_.HandleError("JSON is not an object: " + message);
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
            if (subscribe_pending_) {
                subscribe_pending_ = false;
                subscribe_promise_.set_value(true);
            }
            if (subscribe_trades_pending_) {
                subscribe_trades_pending_ = false;
                subscribe_trades_promise_.set_value(true);
            }
        } else if (event == "unsubscribed") {
            if (unsubscribe_pending_) {
                unsubscribe_pending_ = false;
                unsubscribe_promise_.set_value(true);
            }
            if (unsubscribe_trades_pending_) {
                unsubscribe_trades_pending_ = false;
                unsubscribe_trades_promise_.set_value(true);
            }
        }
    }
}

void BitvavoClient::OnWsError(const std::string& error) {
    if (callbacks_.HandleError) {
        callbacks_.HandleError(error);
    }
}

void BitvavoClient::OnWsConnection(bool connected) {
    if (connected) {
        state_ = ClientState::Connected;
        worker_->StartListening();
    } else {
        state_ = ClientState::Disconnected;
    }

    if (callbacks_.HandleConnection) {
        callbacks_.HandleConnection(connected);
    }
}

void BitvavoClient::HandleTickerEvent(const std::string& message) {
    rapidjson::Document doc;
    doc.Parse(message.c_str());

    if (doc.HasParseError() || !doc.IsObject()) {
        if (callbacks_.HandleError) {
            callbacks_.HandleError("Invalid ticker JSON: " + message);
        }
        return;
    }

    // Market field is required
    if (!doc.HasMember("market") || !doc["market"].IsString()) {
        if (callbacks_.HandleError) {
            callbacks_.HandleError("Missing or invalid 'market' field in ticker: " + message);
        }
        return;
    }

    try {
        BBO bbo;
        bbo.market = doc["market"].GetString();

        // Parse optional bid fields
        if (doc.HasMember("bestBid") && doc["bestBid"].IsString()) {
            bbo.bestBid = std::stod(doc["bestBid"].GetString());
        }
        if (doc.HasMember("bestBidSize") && doc["bestBidSize"].IsString()) {
            bbo.bestBidSize = std::stod(doc["bestBidSize"].GetString());
        }

        // Parse optional ask fields
        if (doc.HasMember("bestAsk") && doc["bestAsk"].IsString()) {
            bbo.bestAsk = std::stod(doc["bestAsk"].GetString());
        }
        if (doc.HasMember("bestAskSize") && doc["bestAskSize"].IsString()) {
            bbo.bestAskSize = std::stod(doc["bestAskSize"].GetString());
        }

        if (callbacks_.HandleBBO) {
            callbacks_.HandleBBO(bbo);
        }
    } catch (const std::exception& e) {
        if (callbacks_.HandleError) {
            callbacks_.HandleError(std::string("Failed to parse ticker values: ") + e.what());
        }
    }
}

void BitvavoClient::HandleTradeEvent(const std::string& message) {
    rapidjson::Document doc;
    doc.Parse(message.c_str());

    if (doc.HasParseError() || !doc.IsObject()) {
        if (callbacks_.HandleError) {
            callbacks_.HandleError("Invalid trade JSON: " + message);
        }
        return;
    }

    // Validate required string fields
    const char* required_string_fields[] = {"market", "id", "price", "amount", "side"};
    for (const auto* field : required_string_fields) {
        if (!doc.HasMember(field) || !doc[field].IsString()) {
            if (callbacks_.HandleError) {
                callbacks_.HandleError(std::string("Missing or invalid field '") + field + "' in trade: " + message);
            }
            return;
        }
    }

    // Validate timestamp (should be a number)
    if (!doc.HasMember("timestamp") || !doc["timestamp"].IsInt64()) {
        if (callbacks_.HandleError) {
            callbacks_.HandleError("Missing or invalid field 'timestamp' in trade: " + message);
        }
        return;
    }

    try {
        PublicTrade trade;
        trade.market = doc["market"].GetString();
        trade.id = doc["id"].GetString();
        trade.price = std::stod(doc["price"].GetString());
        trade.amount = std::stod(doc["amount"].GetString());
        trade.side = doc["side"].GetString();
        trade.timestamp = doc["timestamp"].GetInt64();

        if (callbacks_.HandlePublicTrade) {
            callbacks_.HandlePublicTrade(trade);
        }
    } catch (const std::exception& e) {
        if (callbacks_.HandleError) {
            callbacks_.HandleError(std::string("Failed to parse trade values: ") + e.what());
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
    for (const auto& m : markets) {
        markets_array.PushBack(rapidjson::Value(m.c_str(), alloc), alloc);
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
