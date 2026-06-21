# Connector Common

Shared C++20 library for all exchange connectors and market-data orchestrators.

## Contents

| Header | Purpose |
|--------|---------|
| `connectors/BBO.h` | `BBO`, `PublicTrade`, `OrderBook` market data types |
| `connectors/Instrument.h` | `Instrument`, `Listing`, `InstrumentType`, `kNoExpiryDate` |
| `connectors/MarketDataConnector.h` | Virtual interface for exchange connectors |
| `connectors/InstrumentClient.h` | gRPC client for instrument-server with listing cache |

## Market data types

All inbound events carry `instrument_id_` (canonical ID from instrument-server), not exchange symbols.

```cpp
struct BBO {
    int64_t instrument_id_ = 0;
    std::optional<double> best_bid_;
    // ...
};
```

## InstrumentClient

Wraps `instrument.v1.InstrumentService` gRPC API:

```cpp
connectors::InstrumentClient client("localhost:50051");
auto listing = client.ResolveListing(1, "bitvavo");
// listing->venue_symbol_ == "BTC-EUR"
```

Caches:
- `instrument_id` + `venue` → `Listing`
- `venue` + `venue_symbol` → `instrument_id` (reverse lookup for inbound events)

## Build

```bash
mkdir -p build && cd build
cmake ..
cmake --build .
```

Requires: gRPC, Protobuf (system packages or equivalent).

When used as a subdirectory of bitvavo-connector:

```cmake
add_subdirectory(Dependencies/connector-common)
target_link_libraries(my_connector PRIVATE connector_common)
```

## Proto dependency

Proto files live in [`proto/instrument/v1/instrument.proto`](proto/instrument/v1/instrument.proto).

**Keep in sync** with [instrument-server](https://github.com/AirVan21/instrument-server) `proto/instrument/v1/instrument.proto`. When the API changes, update both repos and bump the package version (`instrument.v1` → `instrument.v2` for breaking changes).

## Implementing a new connector

1. Depend on `connector_common` + `connector-network`
2. Implement `MarketDataConnector` with a `Venue()` returning your exchange name
3. In `SubscribeBBO` / `SubscribeTrades`, call `InstrumentClient::ResolveListing` to get `venue_symbol`
4. On inbound events, set `instrument_id_` via `InstrumentClient::ResolveInstrumentId`
5. Register listings in instrument-server seed data

## Related repos

| Repo | Role |
|------|------|
| instrument-server | Authoritative instrument/listing store |
| bitvavo-connector | Reference implementation |
| connector-network | Low-level WebSocket transport |
