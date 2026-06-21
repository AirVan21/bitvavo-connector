#pragma once

#include <cstdint>
#include <future>
#include <string>
#include <vector>

namespace connectors {

/// @brief Abstract interface for exchange market-data connectors.
///
/// Implementations accept canonical instrument_ids, resolve them to venue symbols
/// internally via InstrumentClient, and deliver events with @p instrument_id_ set
/// on all market-data structs (BBO, PublicTrade, OrderBook).
///
/// All futures resolve to @p true on success, @p false on failure
/// (not connected, unknown instrument, etc.).
/// All callbacks execute on the connector's io_context thread — do not block inside them.
struct MarketDataConnector {
    virtual ~MarketDataConnector() = default;

    virtual std::future<bool> Connect() = 0;
    virtual void Disconnect() = 0;
    virtual std::future<bool> SubscribeBBO(std::vector<int64_t> instrument_ids) = 0;
    virtual std::future<bool> SubscribeTrades(std::vector<int64_t> instrument_ids) = 0;

    /// @return The venue name as registered in instrument-server (e.g. @p "bitvavo").
    virtual std::string Venue() const = 0;
};

} // namespace connectors
