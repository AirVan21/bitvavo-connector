# Bitvavo Connector

A C++20 WebSocket client for streaming real-time market data from the [Bitvavo](https://bitvavo.com) cryptocurrency exchange. Subscribes by canonical `instrument_id` resolved via [instrument-server](../instrument-server), and delivers events with `instrument_id_` set.

## Features

- Real-time **BBO** (Best Bid/Offer) subscriptions by `instrument_id`
- Real-time **public trades** subscriptions by `instrument_id`
- Integration with **instrument-server** for listing resolution
- Implements `MarketDataConnector` from **connector-common**
- Async WebSocket via Boost.ASIO + SSL/TLS
- Callback-based API with `std::future` for subscription lifecycle

## Platform architecture

```
instrument-server (gRPC + PostgreSQL)
        │
        ▼ ResolveListing(instrument_id, "bitvavo")
connector-common (InstrumentClient, BBO, PublicTrade)
        │
        ▼ SubscribeBBO([1, 2])
BitvavoClient ──► WssWorker ──► Bitvavo WS API
        │
        ▼ BBO(instrument_id=1, ...)
   your application
```

| Repo | Role |
|------|------|
| [instrument-server](../instrument-server) | Canonical instrument and listing registry |
| [connector-common](Dependencies/connector-common) | Shared types and gRPC client |
| **bitvavo-connector** (this repo) | Bitvavo-specific WebSocket connector |
| [connector-network](Dependencies/connector-network) | Low-level WSS transport |

## Data structures

Types are defined in `connector-common`:

```cpp
struct BBO {
    int64_t instrument_id_ = 0;   // canonical ID from instrument-server
    std::optional<double> best_bid_;
    std::optional<double> best_bid_size_;
    std::optional<double> best_ask_;
    std::optional<double> best_ask_size_;
};

struct PublicTrade {
    int64_t instrument_id_ = 0;
    std::string trade_id_;
    double price_;
    double amount_;
    std::string side_;
    int64_t timestamp_;
};
```

## Cloning

```bash
git clone --recursive git@github.com:AirVan21/bitvavo-connector.git
git submodule update --init --recursive
```

Submodules: `connector-network`. `connector-common` is vendored under `Dependencies/`.

## Prerequisites

- **CMake** 3.16+, **C++20** compiler
- **Conan 1.64.1** for OpenSSL and RapidJSON
- **Boost**, **OpenSSL**
- **gRPC + Protobuf** (system packages for connector-common):

```bash
sudo apt-get install -y libboost-all-dev libssl-dev \
    libgrpc++-dev libprotobuf-dev protobuf-compiler-grpc
```

- **instrument-server** running (see [instrument-server README](../instrument-server/README.md))

## Building

```bash
mkdir -p build && cd build
conan install .. --build=missing
cmake ..
cmake --build .
```

Produces `libbitvavo_client.a` and `BitvavoConnector` executable.

## Running

Start instrument-server first, then:

```bash
export INSTRUMENT_SERVER_ADDRESS=localhost:50051   # default
./build/BitvavoConnector
```

Example output:

```
[INSTR] BTC-EUR -> bitvavo:BTC-EUR (listing_id=1)
[INSTR] ETH-EUR -> bitvavo:ETH-EUR (listing_id=4)
[CONN] Connected to Bitvavo WebSocket
[BBO] instrument_id=1 bid=0.44@55100.00 ask=0.51@55101.00
[TRADE] instrument_id=1 sell 0.01@55100.00
```

Press `Ctrl+C` to shut down.

## Callback API

```cpp
#include "BitvavoClient.h"
#include "connectors/InstrumentClient.h"

boost::asio::io_context io_context;
connectors::InstrumentClient instrument_client("localhost:50051");

connectors::BitvavoClient::Callbacks callbacks;
callbacks.handle_bbo_ = [](const connectors::BBO& bbo) { /* ... */ };
callbacks.handle_public_trade_ = [](const connectors::PublicTrade& trade) { /* ... */ };

connectors::BitvavoClient client(io_context, instrument_client, std::move(callbacks));

std::thread io_thread([&]() { io_context.run(); });

client.Connect().get();
client.SubscribeBBO({1, 2}).get();      // BTC-EUR, ETH-EUR by instrument_id
client.SubscribeTrades({1, 2}).get();
```

## Adding a new exchange connector

See the checklist in [connector-common README](Dependencies/connector-common/README.md):

1. Create a new repo (e.g. `binance-connector`)
2. Depend on `connector-common` + `connector-network`
3. Implement `MarketDataConnector` with your `Venue()` name
4. Register listings in instrument-server
5. Resolve `instrument_id` → `venue_symbol` before subscribing

## Dependencies

| Package | Source | Purpose |
|---------|--------|---------|
| Boost | apt/brew | ASIO + Beast |
| OpenSSL | Conan | TLS |
| RapidJSON | Conan | Bitvavo JSON parsing |
| gRPC + Protobuf | apt | instrument-server client |
| connector-common | `Dependencies/` | Shared types + InstrumentClient |
| connector-network | submodule | WssWorker transport |
