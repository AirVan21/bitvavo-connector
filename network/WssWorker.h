#pragma once

#include <memory>
#include <string>
#include <functional>
#include <future>

#include <boost/asio.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>

namespace connectors {

    class WssWorker {
    public:
        // Callback types
        using MessageCallback = std::function<void(const std::string&)>;
        using ErrorCallback = std::function<void(const std::string&)>;
        using ConnectionCallback = std::function<void(bool)>;

        WssWorker(boost::asio::io_context& io_context);
        ~WssWorker();

        // Connection management
        std::future<bool> connect(const std::string& host, const std::string& port, const std::string& path = "/");
        // void disconnect();
        // bool is_connected() const;

        // // Data transmission
        // std::future<bool> send(const std::string& message);

        // // Callback setters
        // void set_message_callback(MessageCallback callback);
        // void set_error_callback(ErrorCallback callback);
        // void set_connection_callback(ConnectionCallback callback);

        // // Start/stop listening for incoming messages
        // void start_listening();
        // void stop_listening();

    private:
        // void handle_read(const boost::system::error_code& ec, std::size_t bytes_transferred);
        void report_error(const std::string& error_message);

        boost::asio::io_context& io_context_;
        std::unique_ptr<boost::beast::websocket::stream<boost::beast::ssl_stream<boost::asio::ip::tcp::socket>>> ws_ptr_;
        std::shared_ptr<boost::asio::ssl::context> ssl_context_;
        
        // Callbacks
        MessageCallback message_callback_;
        ErrorCallback error_callback_;
        ConnectionCallback connection_callback_;

        // State
        bool connected_;
        bool listening_;
        std::string host_;
        std::string port_;
        std::string path_;

        // Buffer for reading
        boost::beast::flat_buffer read_buffer_;
    };

}// namespace connectors