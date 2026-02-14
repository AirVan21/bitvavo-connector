#include <atomic>
#include <csignal>
#include <iomanip>
#include <iostream>
#include <thread>

#include <boost/asio.hpp>

#include "BitvavoClient.h"

static std::atomic<bool> g_running{true};

void SignalHandler(int) {
    g_running = false;
}

int main() {
    std::cout << "Bitvavo BBO Connector" << std::endl;

    std::signal(SIGINT, SignalHandler);

    boost::asio::io_context io_context;
    auto work_guard = boost::asio::make_work_guard(io_context);

    connectors::BitvavoClient::Callbacks callbacks;
    callbacks.handle_bbo_ = [](const connectors::BBO& bbo) {
        std::cout << std::fixed << std::setprecision(2) << "[BBO] " << bbo.market_;

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
                  << "[TRADE] " << trade.market_
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

    connectors::BitvavoClient client(io_context, std::move(callbacks));

    // Run io_context on a background thread
    std::thread io_thread([&io_context]() {
        io_context.run();
    });

    // Connect
    auto connect_future = client.Connect();
    if (!connect_future.get()) {
        std::cerr << "Failed to connect" << std::endl;
        work_guard.reset();
        io_context.stop();
        io_thread.join();
        return 1;
    }

    // Subscribe to ticker for BTC-EUR and ETH-EUR
    auto sub_future = client.SubscribeTicker({"BTC-EUR", "ETH-EUR"});
    if (!sub_future.get()) {
        std::cerr << "Failed to subscribe to ticker" << std::endl;
        client.Disconnect();
        work_guard.reset();
        io_context.stop();
        io_thread.join();
        return 1;
    }

    // Subscribe to trades for BTC-EUR and ETH-EUR
    auto trades_future = client.SubscribeTrades({"BTC-EUR", "ETH-EUR"});
    if (!trades_future.get()) {
        std::cerr << "Failed to subscribe to trades" << std::endl;
        client.Disconnect();
        work_guard.reset();
        io_context.stop();
        io_thread.join();
        return 1;
    }

    std::cout << "Subscribed to ticker and trades. Streaming updates (Ctrl+C to quit)..." << std::endl;

    // Wait for SIGINT
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
