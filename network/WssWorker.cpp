#include "WssWorker.h"

#include <iostream>

namespace connectors {

WssWorker::WssWorker(boost::asio::io_context& io_context)
    : io_context_(io_context)
    , connected_(false)
    , listening_(false) {
    // Create SSL context
    ssl_context_ = std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12_client);
    
    // Create the WebSocket stream with SSL
    ws_ptr_ = std::make_unique<boost::beast::websocket::stream<boost::beast::ssl_stream<boost::asio::ip::tcp::socket>>>(
        io_context, *ssl_context_);
}

WssWorker::~WssWorker() {
    Disconnect();
}

std::future<bool> WssWorker::Connect(const ConnectionSettings& settings) {
    connection_settings_ = settings;
    
    return std::async(std::launch::async, [this]() {
        try {
            // Set SNI hostname
            if (!SSL_set_tlsext_host_name(ws_ptr_->next_layer().native_handle(), connection_settings_.host.c_str())) {
                boost::beast::error_code ec{static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category()};
                throw boost::beast::system_error{ec};
            }

            // Look up the domain name
            boost::asio::ip::tcp::resolver resolver{io_context_};
            auto const results = resolver.resolve(connection_settings_.host, connection_settings_.port);

            // Make the connection on the IP address we get from a lookup
            boost::asio::connect(ws_ptr_->next_layer().next_layer(), results.begin(), results.end());

            // Perform the SSL handshake
            ws_ptr_->next_layer().handshake(boost::asio::ssl::stream_base::client);

            // Set a decorator to change the User-Agent of the handshake
            ws_ptr_->set_option(
                boost::beast::websocket::stream_base::decorator(
                    [](boost::beast::websocket::request_type& req) {
                        req.set(boost::beast::http::field::user_agent,
                        std::string(BOOST_BEAST_VERSION_STRING) + " websocket-client-coro");
                    }
                )
            );

            // Perform the WebSocket handshake
            ws_ptr_->handshake(connection_settings_.host, connection_settings_.path);

            connected_ = true;
            if (connection_callback_) {
                connection_callback_(true);
            }
            
            return true;
        } catch (const std::exception& e) {
            ReportError("Connection failed: " + std::string(e.what()));
            connected_ = false;
            if (connection_callback_) {
                connection_callback_(false);
            }
            return false;
        }
    });
}

void WssWorker::Disconnect() {
    if (connected_ && ws_ptr_) {
        try {
            StopListening();
            ws_ptr_->close(boost::beast::websocket::close_code::normal);
            connected_ = false;
            if (connection_callback_) {
                connection_callback_(false);
            }
        } catch (const std::exception& e) {
            ReportError("Disconnect error: " + std::string(e.what()));
        }
    }
}

bool WssWorker::IsConnected() const {
    return connected_;
}

std::future<bool> WssWorker::Send(const std::string& message) {
    return std::async(std::launch::async, [this, message]() {
        if (!connected_ || !ws_ptr_) {
            ReportError("Cannot send: not connected");
            return false;
        }

        try {
            ws_ptr_->write(boost::asio::buffer(message));
            return true;
        } catch (const std::exception& e) {
            ReportError("Send error: " + std::string(e.what()));
            return false;
        }
    });
}

void WssWorker::SetMessageCallback(MessageCallback callback) {
    message_callback_ = callback;
}

void WssWorker::SetErrorCallback(ErrorCallback callback) {
    error_callback_ = callback;
}

void WssWorker::SetConnectionCallback(ConnectionCallback callback) {
    connection_callback_ = callback;
}

void WssWorker::StartListening() {
    if (!connected_ || !ws_ptr_ || listening_) {
        return;
    }

    listening_ = true;
    read_buffer_.clear();
    
    // Start reading asynchronously
    ws_ptr_->async_read(
        read_buffer_,
        [this](const boost::system::error_code& ec, std::size_t bytes_transferred) {
            HandleRead(ec, bytes_transferred);
        });
}

void WssWorker::StopListening() {
    listening_ = false;
}

void WssWorker::HandleRead(const boost::system::error_code& ec, std::size_t bytes_transferred) {
    if (ec) {
        if (ec == boost::beast::websocket::error::closed) {
            // Normal closure
            connected_ = false;
            listening_ = false;
            if (connection_callback_) {
                connection_callback_(false);
            }
        } else {
            ReportError("Read error: " + ec.message());
            connected_ = false;
            listening_ = false;
            if (connection_callback_) {
                connection_callback_(false);
            }
        }
        return;
    }

    // Process the received message
    if (message_callback_) {
        std::string message = boost::beast::buffers_to_string(read_buffer_.data());
        message_callback_(message);
    }

    // Clear the buffer and continue reading if still listening
    read_buffer_.clear();
    if (listening_ && connected_) {
        ws_ptr_->async_read(
            read_buffer_,
            [this](const boost::system::error_code& ec, std::size_t bytes_transferred) {
                HandleRead(ec, bytes_transferred);
            });
    }
}

void WssWorker::ReportError(const std::string& error_message) {
    if (error_callback_) {
        error_callback_(error_message);
    } else {
        std::cerr << "WssWorker error: " << error_message << std::endl;
    }
}

}// namespace connectors