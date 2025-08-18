#include <iostream>
#include <thread>
#include <chrono>

#include "network/WssWorker.h"

using namespace connectors;

int main() {
    std::cout << "Welcome to the Bitvavo Connector!" << std::endl;

    boost::asio::io_context io_context;
    
    // Create WebSocket worker
    WssWorker wss_worker(io_context);
    
    // Set up callbacks
    wss_worker.SetMessageCallback([](const std::string& message) {
        std::cout << "Received message: " << message << std::endl;
    });
    
    wss_worker.SetErrorCallback([](const std::string& error) {
        std::cerr << "WebSocket error: " << error << std::endl;
    });
    
    wss_worker.SetConnectionCallback([](bool connected) {
        if (connected) {
            std::cout << "WebSocket connected successfully!" << std::endl;
        } else {
            std::cout << "WebSocket disconnected." << std::endl;
        }
    });
    
    // Example: Connect to a WebSocket server
    // Replace with your actual WebSocket server details
    ConnectionSettings settings("ws.bitvavo.com", "443", "/v2");  // Example echo server
    
    std::cout << "Connecting to " << settings.host << ":" << settings.port << settings.path << std::endl;
    
    // Connect asynchronously
    auto connect_future = wss_worker.Connect(settings);
    
    // Wait for connection
    if (connect_future.get()) {
        std::cout << "Connection established!" << std::endl;

        // Start listening for incoming messages
        wss_worker.StartListening();

        // Send message
        std::string test_message = "{\n"
                                   "  \"action\": \"getTickerPrice\","
                                   "  \"requestId\": 1,"
                                   "  \"market\": \"BTC-EUR\""
                                   "}";
        auto send_future = wss_worker.Send(test_message);
        
        if (send_future.get()) {
            std::cout << "Message sent successfully!" << std::endl;
        } else {
            std::cout << "Failed to send message." << std::endl;
        }
    } else {
        std::cout << "Failed to connect to WebSocket server." << std::endl;
    }
    
    // Run the io_context
    io_context.run();

    return 0;
}