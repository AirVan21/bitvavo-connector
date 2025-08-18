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

    class WssWorker {
    public:
        // Callback types
        using MessageCallback = std::function<void(const std::string&)>;
        using ErrorCallback = std::function<void(const std::string&)>;
        using ConnectionCallback = std::function<void(bool)>;

        WssWorker(boost::asio::io_context& io_context);
        ~WssWorker();

        // Connection management
        std::future<bool> Connect(const ConnectionSettings& settings);
        void Disconnect();

        // Data transmission
        std::future<bool> Send(const std::string& message);

        // Callback setters
        void SetMessageCallback(MessageCallback callback);
        void SetErrorCallback(ErrorCallback callback);
        void SetConnectionCallback(ConnectionCallback callback);

        // Start/stop listening for incoming messages
        void StartListening();
        void StopListening();

    private:
        void HandleRead(const boost::system::error_code& ec, std::size_t bytes_transferred);
        void ReportError(const std::string& error_message);

        boost::asio::io_context& io_context_;
        std::shared_ptr<boost::asio::ssl::context> ssl_context_;
        std::unique_ptr<boost::beast::websocket::stream<boost::beast::ssl_stream<boost::asio::ip::tcp::socket>>> ws_ptr_;
        
        // Callbacks
        MessageCallback message_callback_;
        ErrorCallback error_callback_;
        ConnectionCallback connection_callback_;

        // State
        ConnectionSettings connection_settings_;
        bool connected_;
        bool listening_;

        // Buffer for reading
        boost::beast::flat_buffer read_buffer_;
    };

}// namespace connectors