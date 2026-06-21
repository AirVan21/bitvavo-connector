#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace connectors {

struct BBO {
    int64_t instrument_id_ = 0;
    std::optional<double> best_bid_;
    std::optional<double> best_bid_size_;
    std::optional<double> best_ask_;
    std::optional<double> best_ask_size_;
};

struct PublicTrade {
    int64_t instrument_id_ = 0;
    std::string trade_id_;
    double price_ = 0.0;
    double amount_ = 0.0;
    std::string side_;
    int64_t timestamp_ = 0;
};

struct OrderBookEntry {
    double price_ = 0.0;
    double size_ = 0.0;
};

struct OrderBook {
    int64_t instrument_id_ = 0;
    int64_t nonce_ = 0;
    std::vector<OrderBookEntry> bids_;
    std::vector<OrderBookEntry> asks_;
};

} // namespace connectors
