#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace connectors {

/**
 * @brief Best Bid/Offer snapshot for a single instrument.
 *
 * Fields are optional because Bitvavo sends partial updates —
 * only the side(s) that changed are included in a given event.
 */
struct BBO {
    int64_t instrument_id_ = 0;       /**< Canonical instrument id from instrument-server. */
    std::optional<double> best_bid_;
    std::optional<double> best_bid_size_;
    std::optional<double> best_ask_;
    std::optional<double> best_ask_size_;
};

/** @brief A single publicly visible trade. */
struct PublicTrade {
    int64_t instrument_id_ = 0; /**< Canonical instrument id from instrument-server. */
    std::string trade_id_;      /**< Exchange-assigned trade identifier. */
    double price_ = 0.0;
    double amount_ = 0.0;
    std::string side_;          /**< @p "buy" or @p "sell" from the taker's perspective. */
    int64_t timestamp_ = 0;     /**< UTC milliseconds. */
};

/** @brief A single price level in an order book snapshot or delta. */
struct OrderBookEntry {
    double price_ = 0.0;
    double size_ = 0.0; /**< Size of zero means the level was removed. */
};

/** @brief Full or incremental order book update. */
struct OrderBook {
    int64_t instrument_id_ = 0; /**< Canonical instrument id from instrument-server. */
    int64_t nonce_ = 0;         /**< Sequence number for ordering incremental updates. */
    std::vector<OrderBookEntry> bids_;
    std::vector<OrderBookEntry> asks_;
};

} // namespace connectors
