# Bitvavo Connector

A C++20 WebSocket client for streaming real-time market data from the [Bitvavo](https://bitvavo.com) cryptocurrency exchange. Built on Boost.Beast + OpenSSL with an async callback-based API.

## Features

- Real-time **BBO** (Best Bid/Offer) ticker subscriptions
- Real-time **public trades** subscriptions
- Async WebSocket connection via Boost.ASIO
- SSL/TLS encryption
- Callback-based API with `std::future` for subscription lifecycle
- Usable as a **standalone executable** or as a **static library** for embedding in other projects

## Architecture

```
┌──────────────────────────────────────────────────┐
│                   BitvavoClient                   │
│                                                   │
│  Connect() ──► WssWorker (Boost.Beast + OpenSSL) │
│                    │                              │
│  SubscribeTicker() │  JSON subscribe/unsubscribe  │
│  SubscribeTrades() │  via Bitvavo WS v2 API       │
│                    ▼                              │
│  Callbacks:   handle_bbo_          ◄── ticker     │
│               handle_public_trade_ ◄── trades     │
│               handle_error_        ◄── errors     │
│               handle_connection_   ◄── connect    │
└──────────────────────────────────────────────────┘
```

### Data Flow

1. `BitvavoClient` connects to `wss://ws.bitvavo.com/v2/` via `WssWorker`
2. Subscriptions send JSON payloads; server ACKs resolve `std::future<bool>`
3. Incoming ticker events are parsed (RapidJSON) into `BBO` structs
4. Incoming trade events are parsed into `PublicTrade` structs
5. All callbacks execute on the `io_context` thread

### Data Structures

```cpp
struct BBO {
    std::string market_;                  // e.g. "BTC-EUR"
    std::optional<double> best_bid_;
    std::optional<double> best_bid_size_;
    std::optional<double> best_ask_;
    std::optional<double> best_ask_size_;
};

struct PublicTrade {
    std::string market_;
    std::string id_;
    double price_;
    double amount_;
    std::string side_;        // "buy" or "sell"
    int64_t timestamp_;       // UTC milliseconds
};
```

## Cloning

This project uses [connector-network](https://github.com/AirVan21/connector-network) as a submodule.

```bash
git clone --recursive git@github.com:AirVan21/bitvavo-connector.git
```

Or if already cloned:

```bash
git submodule update --init --recursive
```

## Prerequisites

- **CMake** 3.16+
- **C++20** compiler (GCC 10+, Clang 12+)
- **Conan 1.64.1** (not Conan 2.x) — `pip install conan==1.64.1`
- **Boost** (ASIO + Beast) and **OpenSSL**

```bash
# Ubuntu / Debian
sudo apt-get install libboost-all-dev libssl-dev

# macOS
brew install boost openssl
```

## Building

### Using Docker

```bash
docker build -t bitvavo-connector .
docker run -it --name bitvavo-connector -v $(pwd):/bitvavo-connector bitvavo-connector
# Inside container
mkdir build && cd build
conan install .. --build=missing
cmake ..
cmake --build .
```

To push commits from inside the container, forward your SSH agent:

```bash
docker run -it \
  -v $(pwd):/bitvavo-connector \
  -v $SSH_AUTH_SOCK:/ssh-agent \
  -e SSH_AUTH_SOCK=/ssh-agent \
  bitvavo-connector
```

### Local Build

```bash
mkdir -p build && cd build
conan install .. --build=missing
cmake ..
cmake --build .
```

This produces:
- `libbitvavo_client.a` — static library
- `BitvavoConnector` — standalone executable

## Running

```bash
./build/BitvavoConnector
```

Connects to Bitvavo, subscribes to BTC-EUR and ETH-EUR ticker + trades, and streams updates to stdout:

```
[CONN] Connected to Bitvavo WebSocket
[BBO] BTC-EUR bid=1.50@45000.00 ask=0.80@45010.00
[TRADE] BTC-EUR buy 0.25@45005.00
```

Press `Ctrl+C` to shut down gracefully.

## Using as a Library

The project exposes a `bitvavo_client` CMake static library target for use as a git submodule in other projects (e.g. [market-data-recorder](https://github.com/AirVan21/market-data-recorder)).

### Integration

```bash
git submodule add https://github.com/AirVan21/bitvavo-connector.git Dependencies/bitvavo-connector
```

```cmake
# CMakeLists.txt
add_subdirectory(Dependencies/bitvavo-connector)

add_executable(MyApp main.cpp)
target_link_libraries(MyApp PRIVATE bitvavo_client)
```

The `bitvavo_client` target transitively provides all required include paths (Boost, OpenSSL, RapidJSON) and link libraries.

### Callback API

```cpp
#include "BitvavoClient.h"

boost::asio::io_context io_context;

connectors::BitvavoClient::Callbacks callbacks;
callbacks.handle_bbo_ = [](const connectors::BBO& bbo) {
    // Handle BBO update
};
callbacks.handle_public_trade_ = [](const connectors::PublicTrade& trade) {
    // Handle trade
};
callbacks.handle_error_ = [](const std::string& error) {
    // Handle error
};
callbacks.handle_connection_ = [](bool connected) {
    // Handle connection state change
};

connectors::BitvavoClient client(io_context, std::move(callbacks));

// Run io_context on a background thread
std::thread io_thread([&]() { io_context.run(); });

// Connect and subscribe
client.Connect().get();
client.SubscribeTicker({"BTC-EUR", "ETH-EUR"}).get();
client.SubscribeTrades({"BTC-EUR", "ETH-EUR"}).get();
```

## Dependencies

| Package | Version | Source | Purpose |
|---|---|---|---|
| Boost | system | apt/brew | ASIO + Beast (async I/O, WebSocket) |
| OpenSSL | 1.1.1t | Conan | TLS for WebSocket connection |
| RapidJSON | cci.20220822 | Conan | JSON parsing of Bitvavo API messages |
| [connector-network](https://github.com/AirVan21/connector-network) | — | Git submodule | Low-level WebSocket client (WssWorker) |
