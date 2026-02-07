# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

This is a C++20 CMake project using Conan 1.x for dependency management.

```bash
# Full build (from repo root)
mkdir -p build && cd build
conan install .. --build=missing
cmake ..
cmake --build .

# Release build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .

# Docker-based build
docker build -t bitvavo-connector .
docker run -it -v $(pwd):/bitvavo-connector bitvavo-connector
```

The built executable is `build/BitvavoConnector`.

**Cloning:** requires `--recursive` for the connector-network submodule, or run `git submodule update --init --recursive` after cloning.

**No tests or linter are configured in this project.**

## Dependencies

- **Conan 1.64.1** (not Conan 2.x) — manages OpenSSL 1.1.1t and RapidJSON
- **System packages:** Boost (ASIO + Beast), OpenSSL, CMake 3.16+
- **macOS:** `brew install boost openssl`
- **Ubuntu:** `apt-get install libboost-all-dev libssl-dev`

## Architecture

The project is a WebSocket client that connects to the Bitvavo cryptocurrency exchange and streams real-time Best Bid/Offer (BBO) market data.

### Key Components

- **`BitvavoClient`** (`BitvavoClient.h/.cpp`) — High-level client wrapping the WebSocket layer. Manages connection lifecycle, ticker subscriptions, and JSON parsing of Bitvavo API messages. Uses RapidJSON to parse incoming ticker events into `BBO` structs and to build subscribe/unsubscribe JSON payloads. Exposes a callback-based API (`SetBBOCallback`, `SetErrorCallback`, `SetConnectionCallback`).

- **`WssWorker`** (`Dependencies/connector-network/wss/WssWorker.h/.cpp`) — Low-level secure WebSocket client from the `connector-network` submodule. Built on Boost.Beast + OpenSSL. Provides async connect, send, and receive with a callback interface (`Callbacks` struct with `on_message`, `on_error`, `on_connection`).

- **`main.cpp`** — Entry point. Sets up `boost::asio::io_context` on a background thread, creates `BitvavoClient`, subscribes to BTC-EUR and ETH-EUR tickers, and streams BBO updates until SIGINT.

### Data Flow

1. `main` creates `BitvavoClient` with a shared `io_context`
2. `BitvavoClient::Connect()` creates a `WssWorker` and connects to `ws.bitvavo.com:443/v2/`
3. On connection, `WssWorker::StartListening()` begins async reads
4. `SubscribeTicker()` sends a JSON subscription message; server ACK resolves the returned `std::future<bool>`
5. Incoming ticker events are parsed in `HandleTickerEvent()` and delivered via `BBOCallback`

All async I/O runs on the `io_context` thread; callbacks execute in that thread context. Everything lives in the `connectors` namespace.
