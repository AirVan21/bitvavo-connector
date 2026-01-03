#pragma once

#include <memory>
#include <string>
#include <functional>
#include <future>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>

namespace connectors {

    struct ConnectionSettings {
        std::string host;
        std::string port;
        std::string path = "/";
    };

    struct Callbacks {
        std::function<void(const std::string&)> on_message;
        std::function<void(const std::string&)> on_error;
        std::function<void(bool)> on_connection;
    };

    struct WssWorker {
        WssWorker(boost::asio::io_context& io_context, Callbacks callbacks = {});
        ~WssWorker();

        // Delete copy constructor and copy assignment operator
        WssWorker(const WssWorker&) = delete;
        WssWorker& operator=(const WssWorker&) = delete;

        // Connection management
        std::future<bool> Connect(const ConnectionSettings& settings);
        void Disconnect();

        // Data transmission
        std::future<bool> Send(const std::string& message);

        // Start/stop listening for incoming messages
        void StartListening();
        void StopListening();

    private:
        void HandleRead(const boost::system::error_code& ec, std::size_t bytes_transferred);
        void ReportError(const std::string& error_message);

        boost::asio::io_context& io_context_;
        boost::asio::ssl::context ssl_context_;
        std::unique_ptr<boost::beast::websocket::stream<boost::beast::ssl_stream<boost::asio::ip::tcp::socket>>> ws_ptr_;
        
        // Callbacks
        Callbacks callbacks_;

        // State
        ConnectionSettings connection_settings_;
        bool connected_;
        bool listening_;

        // Buffer for reading
        boost::beast::flat_buffer read_buffer_;
    };

}// namespace connectors