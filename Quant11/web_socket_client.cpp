#include "web_socket_client.h"
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>
#include <json/json.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class WebSocketHandler {
public:
    WebSocketClient* client;
    std::string symbol;
    
    WebSocketHandler(WebSocketClient* c, const std::string& s) : client(c), symbol(s) {}
    
    void handle_message(connection_hdl hdl, client::message_ptr msg) {
        if (msg->get_opcode() == websocketpp::frame::opcode::text) {
            client->HandleMessage(msg->get_payload());
        }
    }
    
    void handle_open(connection_hdl hdl) {
        client->is_connected = true;
        client->connection_hdl = hdl;
        std::cout << client->GetFormattedTimestamp() << " Connected!\n";
        client->SubscribeToSymbol(symbol);
    }
    
    void handle_close(connection_hdl hdl) {
        client->is_connected = false;
        std::cout << client->GetFormattedTimestamp() << " Disconnected!\n";
    }
    
    void handle_fail(connection_hdl hdl) {
        client->is_connected = false;
        std::cerr << client->GetFormattedTimestamp() << " Connection failed!\n";
    }
};

WebSocketClient::WebSocketClient() : m_connected(false)
{
    m_client.init_asio();
    
    m_client.set_tls_init_handler([](const char* hostname, connection_hdl) {
        return websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12);
    });

    m_client.set_message_handler([this](connection_hdl hdl, message_ptr msg) {
        on_message(hdl, msg);
    });

    m_client.set_open_handler([this](connection_hdl hdl) {
        on_open(hdl);
    });

    m_client.set_close_handler([this](connection_hdl hdl) {
        on_close(hdl);
    });

    m_client.set_fail_handler([this](connection_hdl hdl) {
        on_fail(hdl);
    });
}

WebSocketClient::~WebSocketClient()
{
    disconnect();
}

context_ptr WebSocketClient::OnTLSInit(const char* hostname, connection_hdl)
{
    context_ptr ctx = websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12);
    try {
        ctx->set_options(boost::asio::ssl::context::default_workarounds |
                        boost::asio::ssl::context::no_sslv2 |
                        boost::asio::ssl::context::no_sslv3 |
                        boost::asio::ssl::context::no_tlsv1 |
                        boost::asio::ssl::context::no_tlsv1_1 |
                        boost::asio::ssl::context::single_dh_use);
        ctx->set_verify_mode(boost::asio::ssl::verify_none);
    } catch (std::exception& e) {
        std::cerr << GetFormattedTimestamp() << " Error in TLS context initialization: " << e.what() << "\n";
    }
    return ctx;
}

std::string WebSocketClient::GetFormattedTimestamp()
{
    const auto now = std::chrono::system_clock::now();
    const auto now_time = std::chrono::system_clock::to_time_t(now);
    const auto ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;

    struct tm timeinfo;
    char timestamp[20];
    localtime_r(&now_time, &timeinfo);
    std::strftime(timestamp, sizeof(timestamp), "%H:%M:%S", &timeinfo);

    std::stringstream ss;
    ss << timestamp << "." << std::setfill('0') << std::setw(3) << ms;
    return ss.str();
}

void WebSocketClient::ConnectToServer(const std::string& symbol)
{
    ws_symbol = symbol;

    try {
        std::cout << GetFormattedTimestamp() << " Connecting to Deribit WebSocket...\n";
        
        WebSocketHandler handler(this, symbol);
        
        m_client.set_message_handler([&handler](connection_hdl hdl, message_ptr msg) {
            handler.handle_message(hdl, msg);
        });
        
        m_client.set_open_handler([&handler](connection_hdl hdl) {
            handler.handle_open(hdl);
        });
        
        m_client.set_close_handler([&handler](connection_hdl hdl) {
            handler.handle_close(hdl);
        });
        
        m_client.set_fail_handler([&handler](connection_hdl hdl) {
            handler.handle_fail(hdl);
        });
        
        websocketpp::lib::error_code ec;
        client::connection_ptr con = m_client.get_connection("wss://test.deribit.com/ws/api/v2", ec);
        
        if (ec) {
            std::cerr << GetFormattedTimestamp() << " Connection initialization error: " << ec.message() << "\n";
            return;
        }

        m_client.connect(con);
        m_client.run();
    }
    catch (const std::exception& e) {
        std::cerr << GetFormattedTimestamp() << " Exception: " << e.what() << "\n";
    }
}

void WebSocketClient::SubscribeToSymbol(const std::string& symbol)
{
    try {
        Json::Value msg;
        msg["jsonrpc"] = "2.0";
        msg["method"] = "public/subscribe";
        msg["params"]["channels"] = Json::Value(Json::arrayValue);
        msg["params"]["channels"].append("book." + symbol + ".100ms");
        msg["id"] = 0;

        const Json::StreamWriterBuilder writer;
        const std::string msg_str = Json::writeString(writer, msg);

        if (m_connected) {
            websocketpp::lib::error_code ec;
            m_client.send(connection_hdl, msg_str, websocketpp::frame::opcode::text, ec);
            if (ec) {
                std::cerr << GetFormattedTimestamp() << " Error sending message: " << ec.message() << "\n";
            } else {
                std::cout << GetFormattedTimestamp() << " Subscription request sent for: " << symbol << "\n";
            }
        } else {
            std::cerr << GetFormattedTimestamp() << " WebSocket connection is not established.\n";
        }
    }
    catch (const std::exception& e) {
        std::cerr << GetFormattedTimestamp() << " Exception during subscription: " << e.what() << "\n";
    }
}

void WebSocketClient::HandleMessage(const std::string& msg)
{
    try {
        Json::Value json_data;
        const Json::CharReaderBuilder reader_builder;
        std::string errs;
        std::istringstream s(msg);

        if (Json::parseFromStream(reader_builder, s, &json_data, &errs)) {
            if (json_data.isMember("params")) {
                const auto& params = json_data["params"];
                if (params.isMember("channel") && params.isMember("data")) {
                    const std::string channel = params["channel"].asString();
                    const auto& data = params["data"];

                    if (channel.find("book.") != std::string::npos) {
                        std::cout << GetFormattedTimestamp() << " Order Book Update:\n";
                        if (data.isMember("bids") && data.isMember("asks")) {
                            std::cout << "Bids:\n";
                            for (const auto& bid : data["bids"]) {
                                std::cout << "Price: " << bid[1].asString() << " Size: " << bid[2].asString() << "\n";
                            }
                            std::cout << "Asks:\n";
                            for (const auto& ask : data["asks"]) {
                                std::cout << "Price: " << ask[1].asString() << " Size: " << ask[2].asString() << "\n";
                            }
                        }
                    }
                }
            }
        } else {
            std::cerr << GetFormattedTimestamp() << " Failed to parse message: " << errs << "\n";
        }
    }
    catch (const std::exception& e) {
        std::cerr << GetFormattedTimestamp() << " Exception processing message: " << e.what() << "\n";
    }
}

bool WebSocketClient::connect(const std::string& uri) {
    try {
        websocketpp::lib::error_code ec;
        auto con = m_client.get_connection(uri, ec);
        if (ec) {
            std::cerr << "Connection error: " << ec.message() << std::endl;
            return false;
        }

        m_client.connect(con);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error connecting: " << e.what() << std::endl;
        return false;
    }
}

void WebSocketClient::disconnect() {
    try {
        if (m_connected) {
            m_client.close(connection_hdl, websocketpp::close::status::going_away, "");
        }
    } catch (const std::exception& e) {
        std::cerr << "Error disconnecting: " << e.what() << std::endl;
    }
}

void WebSocketClient::send_message(const std::string& message) {
    try {
        if (m_connected) {
            m_client.send(connection_hdl, message, websocketpp::frame::opcode::text);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error sending message: " << e.what() << std::endl;
    }
}

std::string WebSocketClient::receive_message() {
    std::unique_lock<std::mutex> lock(m_queue_mutex);
    m_queue_cv.wait(lock, [this] { return !m_message_queue.empty(); });
    std::string message = m_message_queue.front();
    m_message_queue.pop();
    return message;
}

bool WebSocketClient::is_connected() const {
    std::lock_guard<std::mutex> lock(m_connected_mutex);
    return m_connected;
}

void WebSocketClient::on_message(connection_hdl hdl, message_ptr msg) {
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    m_message_queue.push(msg->get_payload());
    m_queue_cv.notify_one();
}

void WebSocketClient::on_open(connection_hdl hdl) {
    std::lock_guard<std::mutex> lock(m_connection_mutex);
    connection_hdl = hdl;
    {
        std::lock_guard<std::mutex> connected_lock(m_connected_mutex);
        m_connected = true;
    }
}

void WebSocketClient::on_close(connection_hdl hdl) {
    std::lock_guard<std::mutex> lock(m_connection_mutex);
    connection_hdl.reset();
    {
        std::lock_guard<std::mutex> connected_lock(m_connected_mutex);
        m_connected = false;
    }
}

void WebSocketClient::on_fail(connection_hdl hdl) {
    std::cerr << "Connection failed" << std::endl;
    on_close(hdl);
}

