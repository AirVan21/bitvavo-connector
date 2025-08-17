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
    ConnectionSettings settings("echo.websocket.org", "443", "/");  // Example echo server
    
    std::cout << "Connecting to " << settings.host << ":" << settings.port << settings.path << std::endl;
    
    // Connect asynchronously
    auto connect_future = wss_worker.Connect(settings);
    
    // Wait for connection
    if (connect_future.get()) {
        std::cout << "Connection established!" << std::endl;
        
        // Start listening for incoming messages
        wss_worker.StartListening();
        
        // Send a test message
        std::string test_message = "Hello, WebSocket!";
        std::cout << "Sending message: " << test_message << std::endl;
        
        auto send_future = wss_worker.Send(test_message);
        if (send_future.get()) {
            std::cout << "Message sent successfully!" << std::endl;
        } else {
            std::cout << "Failed to send message." << std::endl;
        }
        
        // Keep the connection alive for a few seconds to receive the echo
        std::this_thread::sleep_for(std::chrono::seconds(3));
        
        // Disconnect
        wss_worker.Disconnect();
        std::cout << "Disconnected from WebSocket server." << std::endl;
    } else {
        std::cout << "Failed to connect to WebSocket server." << std::endl;
    }
    
    // Run the io_context
    io_context.run();

    return 0;
}