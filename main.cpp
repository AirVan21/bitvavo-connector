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
    // wss_worker.set_message_callback([](const std::string& message) {
    //     std::cout << "Received message: " << message << std::endl;
    // });
    
    // wss_worker.set_error_callback([](const std::string& error) {
    //     std::cerr << "WebSocket error: " << error << std::endl;
    // });
    
    // wss_worker.set_connection_callback([](bool connected) {
    //     if (connected) {
    //         std::cout << "WebSocket connected successfully!" << std::endl;
    //     } else {
    //         std::cout << "WebSocket disconnected." << std::endl;
    //     }
    // });
    
    // // Example: Connect to a WebSocket server
    // // Replace with your actual WebSocket server details
    // std::string host = "echo.websocket.org";  // Example echo server
    // std::string port = "443";
    // std::string path = "/";
    
    // std::cout << "Connecting to " << host << ":" << port << path << std::endl;
    
    // // Connect asynchronously
    // auto connect_future = wss_worker.connect(host, port, path);
    
    // // Wait for connection
    // if (connect_future.get()) {
    //     std::cout << "Connection established!" << std::endl;
        
    //     // Start listening for incoming messages
    //     wss_worker.start_listening();
        
    //     // Send a test message
    //     std::string test_message = "Hello, WebSocket!";
    //     std::cout << "Sending message: " << test_message << std::endl;
        
    //     auto send_future = wss_worker.send(test_message);
    //     if (send_future.get()) {
    //         std::cout << "Message sent successfully!" << std::endl;
    //     } else {
    //         std::cout << "Failed to send message." << std::endl;
    //     }
        
    //     // Keep the connection alive for a few seconds to receive the echo
    //     std::this_thread::sleep_for(std::chrono::seconds(3));
        
    //     // Disconnect
    //     wss_worker.disconnect();
    //     std::cout << "Disconnected from WebSocket server." << std::endl;
    // } else {
    //     std::cout << "Failed to connect to WebSocket server." << std::endl;
    // }
    
    // Run the io_context
    io_context.run();

    return 0;
}