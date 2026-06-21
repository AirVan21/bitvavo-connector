#pragma once

#include <cstdint>
#include <future>
#include <string>
#include <vector>

namespace connectors {

// Abstract interface for exchange market-data connectors.
// Implementations receive canonical instrument_ids, resolve them to venue symbols
// internally, and deliver events with instrument_id_ set on all structs.
//
// All futures resolve to true on success, false on failure (not connected, unknown instrument, etc.).
// All callbacks execute on the connector's io_context thread — do not block inside them.
struct MarketDataConnector {
    virtual ~MarketDataConnector() = default;

    virtual std::future<bool> Connect() = 0;
    virtual void Disconnect() = 0;
    virtual std::future<bool> SubscribeBBO(std::vector<int64_t> instrument_ids) = 0;
    virtual std::future<bool> SubscribeTrades(std::vector<int64_t> instrument_ids) = 0;

    // Returns the venue name as registered in instrument-server (e.g. "bitvavo").
    virtual std::string Venue() const = 0;
};

} // namespace connectors
