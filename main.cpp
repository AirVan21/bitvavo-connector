#include <atomic>
#include <csignal>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <thread>

#include <boost/asio.hpp>

#include "BitvavoClient.h"
#include "connectors/InstrumentClient.h"

static std::atomic<bool> g_running{true};

void SignalHandler(int) {
    g_running = false;
}

std::string GetEnvOrDefault(const char* name, const char* default_value) {
    const char* value = std::getenv(name);
    return value ? std::string(value) : std::string(default_value);
}

int main() {
    std::cout << "Bitvavo Connector (instrument-server integrated)" << std::endl;

    std::signal(SIGINT, SignalHandler);

    const std::string instrument_server_address =
        GetEnvOrDefault("INSTRUMENT_SERVER_ADDRESS", "localhost:50051");

    boost::asio::io_context io_context;
    auto work_guard = boost::asio::make_work_guard(io_context);

    connectors::InstrumentClient instrument_client(instrument_server_address);

    connectors::BitvavoClient::Callbacks callbacks;
    callbacks.handle_bbo_ = [](const connectors::BBO& bbo) {
        std::cout << std::fixed << std::setprecision(2)
                  << "[BBO] instrument_id=" << bbo.instrument_id_;

        if (bbo.best_bid_ && bbo.best_bid_size_) {
            std::cout << " bid=" << *bbo.best_bid_size_ << "@" << *bbo.best_bid_;
        }

        if (bbo.best_ask_ && bbo.best_ask_size_) {
            std::cout << " ask=" << *bbo.best_ask_size_ << "@" << *bbo.best_ask_;
        }

        std::cout << std::endl;
    };
    callbacks.handle_public_trade_ = [](const connectors::PublicTrade& trade) {
        std::cout << std::fixed << std::setprecision(2)
                  << "[TRADE] instrument_id=" << trade.instrument_id_
                  << " " << trade.side_
                  << " " << trade.amount_ << "@" << trade.price_
                  << std::endl;
    };
    callbacks.handle_error_ = [](const std::string& error) {
        std::cerr << "[ERROR] " << error << std::endl;
    };
    callbacks.handle_connection_ = [](bool connected) {
        if (connected) {
            std::cout << "[CONN] Connected to Bitvavo WebSocket" << std::endl;
        } else {
            std::cout << "[CONN] Disconnected" << std::endl;
        }
    };

    connectors::BitvavoClient client(io_context, instrument_client, std::move(callbacks));

    std::thread io_thread([&io_context]() {
        io_context.run();
    });

    // Resolve BTC-EUR (instrument_id=1) and ETH-EUR (instrument_id=2) from instrument server
    constexpr int64_t kBtcEurInstrumentId = 1;
    constexpr int64_t kEthEurInstrumentId = 2;

    auto btc_listing = instrument_client.ResolveListing(kBtcEurInstrumentId, "bitvavo");
    auto eth_listing = instrument_client.ResolveListing(kEthEurInstrumentId, "bitvavo");
    if (!btc_listing || !eth_listing) {
        std::cerr << "Failed to resolve listings from instrument server at "
                  << instrument_server_address << std::endl;
        work_guard.reset();
        io_context.stop();
        io_thread.join();
        return 1;
    }

    std::cout << "[INSTR] BTC-EUR -> bitvavo:" << btc_listing->venue_symbol_
              << " (listing_id=" << btc_listing->listing_id_ << ")" << std::endl;
    std::cout << "[INSTR] ETH-EUR -> bitvavo:" << eth_listing->venue_symbol_
              << " (listing_id=" << eth_listing->listing_id_ << ")" << std::endl;

    auto connect_future = client.Connect();
    if (!connect_future.get()) {
        std::cerr << "Failed to connect" << std::endl;
        work_guard.reset();
        io_context.stop();
        io_thread.join();
        return 1;
    }

    const std::vector<int64_t> instrument_ids = {kBtcEurInstrumentId, kEthEurInstrumentId};

    auto bbo_future = client.SubscribeBBO(instrument_ids);
    if (!bbo_future.get()) {
        std::cerr << "Failed to subscribe to BBO" << std::endl;
        client.Disconnect();
        work_guard.reset();
        io_context.stop();
        io_thread.join();
        return 1;
    }

    auto trades_future = client.SubscribeTrades(instrument_ids);
    if (!trades_future.get()) {
        std::cerr << "Failed to subscribe to trades" << std::endl;
        client.Disconnect();
        work_guard.reset();
        io_context.stop();
        io_thread.join();
        return 1;
    }

    std::cout << "Subscribed by instrument_id. Streaming updates (Ctrl+C to quit)..." << std::endl;

    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "\nShutting down..." << std::endl;
    client.Disconnect();
    work_guard.reset();
    io_context.stop();
    io_thread.join();

    return 0;
}
