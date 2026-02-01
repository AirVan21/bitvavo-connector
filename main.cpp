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

    connectors::BitvavoClient client(io_context);

    client.SetBBOCallback([](const connectors::BBO& bbo) {
        std::cout << std::fixed << std::setprecision(2)
                  << "[BBO] " << bbo.market
                  << " bid=" << bbo.bestBid << " (" << bbo.bestBidSize << ")"
                  << " ask=" << bbo.bestAsk << " (" << bbo.bestAskSize << ")"
                  << " last=" << bbo.lastPrice
                  << std::endl;
    });

    client.SetErrorCallback([](const std::string& error) {
        std::cerr << "[ERROR] " << error << std::endl;
    });

    client.SetConnectionCallback([](bool connected) {
        if (connected) {
            std::cout << "[CONN] Connected to Bitvavo WebSocket" << std::endl;
        } else {
            std::cout << "[CONN] Disconnected" << std::endl;
        }
    });

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
        std::cerr << "Failed to subscribe" << std::endl;
        client.Disconnect();
        work_guard.reset();
        io_context.stop();
        io_thread.join();
        return 1;
    }

    std::cout << "Subscribed. Streaming BBO updates (Ctrl+C to quit)..." << std::endl;

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
