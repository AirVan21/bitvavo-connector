#pragma once

#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "connectors/Instrument.h"

namespace connectors {

// gRPC client for instrument-server. Provides instrument and listing lookups
// with an in-process cache to avoid repeated RPCs on the hot path.
// Thread-safe: all public methods may be called concurrently.
struct InstrumentClient {
    explicit InstrumentClient(std::string server_address);
    ~InstrumentClient();

    // Network RPCs — each call hits instrument-server and caches the result.
    std::optional<Instrument> GetInstrument(int64_t instrument_id);
    std::optional<Listing> GetListing(int64_t listing_id);
    std::optional<Listing> ResolveListing(int64_t instrument_id, const std::string& venue);
    std::vector<Listing> ListListings(int64_t instrument_id, bool active_only = true);

    // Cache-only reverse lookup: venue + venue_symbol -> instrument_id.
    // Returns nullopt if the mapping has not been cached yet via ResolveListing or ListListings.
    // Intended for the inbound event hot path where a network call would add latency.
    std::optional<int64_t> ResolveInstrumentId(const std::string& venue,
                                                  const std::string& venue_symbol);

private:
    // Populates both cache maps from a listing. Called after every successful RPC.
    void CacheListing(const Listing& listing);

    std::string server_address_;
    struct InstrumentClientImpl;
    std::unique_ptr<InstrumentClientImpl> impl_;

    std::mutex cache_mutex_;
    std::unordered_map<std::string, Listing> listing_cache_;          // key: "venue:instrument_id"
    std::unordered_map<std::string, int64_t> symbol_to_instrument_id_; // key: "venue:venue_symbol"
};

} // namespace connectors
