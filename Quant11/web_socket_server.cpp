#include "web_socket_server.h"
#include <json/json.h>
#include <iostream>
#include <chrono>

WebSocketServer::WebSocketServer(PerformanceMonitor& monitor) 
    : m_performance_monitor(monitor) {
    // Set up WebSocket++ server
    m_server.clear_access_channels(websocketpp::log::alevel::all);
    m_server.set_access_channels(websocketpp::log::alevel::connect |
                                websocketpp::log::alevel::disconnect |
                                websocketpp::log::alevel::app);

    m_server.init_asio();

    // Set up event handlers
    m_server.set_open_handler(bind(&WebSocketServer::on_open, this, std::placeholders::_1));
    m_server.set_close_handler(bind(&WebSocketServer::on_close, this, std::placeholders::_1));
    m_server.set_message_handler(bind(&WebSocketServer::on_message, this, 
                                    std::placeholders::_1, std::placeholders::_2));
}

WebSocketServer::~WebSocketServer() {
    stop();
}

void WebSocketServer::start(uint16_t port) {
    try {
        m_server.listen(port);
        m_server.start_accept();
        std::cout << "WebSocket server started on port " << port << std::endl;
        m_server.run();
    } catch (const std::exception& e) {
        std::cerr << "Error starting WebSocket server: " << e.what() << std::endl;
    }
}

void WebSocketServer::stop() {
    try {
        m_server.stop_listening();
        
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            for (auto& hdl : m_connections) {
                m_server.close(hdl, websocketpp::close::status::going_away, "Server shutting down");
            }
            m_connections.clear();
            m_subscriptions.clear();
        }
        
        m_server.stop();
    } catch (const std::exception& e) {
        std::cerr << "Error stopping server: " << e.what() << std::endl;
    }
}

void WebSocketServer::broadcast(const std::string& symbol, const std::string& message) {
    auto start = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_subscriptions.find(symbol);
    if (it != m_subscriptions.end()) {
        for (const auto& hdl : it->second) {
            try {
                m_server.send(hdl, message, websocketpp::frame::opcode::text);
            } catch (const std::exception& e) {
                std::cerr << "Error broadcasting message: " << e.what() << std::endl;
            }
        }
    }
    
    auto end = std::chrono::steady_clock::now();
    auto latency = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    m_performance_monitor.record_latency("websocket_broadcast", latency);
}

void WebSocketServer::handle_subscription(connection_hdl hdl, const std::string& symbol) {
    auto start = std::chrono::steady_clock::now();
    
    std::lock_guard<std::mutex> lock(m_mutex);
    m_subscriptions[symbol].insert(hdl);
    std::cout << "New subscription for symbol: " << symbol << std::endl;
    
    auto end = std::chrono::steady_clock::now();
    auto latency = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    m_performance_monitor.record_latency("subscription_handling", latency);
}

void WebSocketServer::remove_connection(connection_hdl hdl) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_connections.erase(hdl);
    for (auto& pair : m_subscriptions) {
        pair.second.erase(hdl);
    }
    m_connection_count--;
}

void WebSocketServer::on_open(connection_hdl hdl) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_connections.insert(hdl);
    m_connection_count++;
    std::cout << "New WebSocket connection established. Total connections: " 
              << m_connection_count << std::endl;
}

void WebSocketServer::on_close(connection_hdl hdl) {
    remove_connection(hdl);
    std::cout << "WebSocket connection closed. Remaining connections: " 
              << m_connection_count << std::endl;
}

void WebSocketServer::on_message(connection_hdl hdl, message_ptr msg) {
    auto start = std::chrono::steady_clock::now();
    
    try {
        Json::Value json_msg;
        Json::Reader reader;
        if (reader.parse(msg->get_payload(), json_msg)) {
            if (json_msg.isMember("type") && json_msg["type"].asString() == "subscribe") {
                if (json_msg.isMember("symbol")) {
                    handle_subscription(hdl, json_msg["symbol"].asString());
                }
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error processing message: " << e.what() << std::endl;
    }

    auto end = std::chrono::steady_clock::now();
    auto latency = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    m_performance_monitor.record_latency("websocket_message_processing", latency);
}

double WebSocketServer::get_average_latency() const {
    if (m_metrics.total_messages == 0) return 0.0;
    return static_cast<double>(m_metrics.total_latency) / m_metrics.total_messages;
}

double WebSocketServer::get_message_rate() const {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - m_start_time).count();
    if (duration == 0) return 0.0;
    return static_cast<double>(m_metrics.total_messages) / duration;
}

uint64_t WebSocketServer::get_total_connections() const {
    return m_connection_count;
}

uint64_t WebSocketServer::get_total_messages() const {
    return m_metrics.total_messages;
}

uint64_t WebSocketServer::get_max_latency() const {
    return m_metrics.max_latency;
} 