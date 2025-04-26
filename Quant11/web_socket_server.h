#pragma once

#include <websocketpp/server.hpp>
#include <websocketpp/config/asio_no_tls.hpp>
#include "performance_monitor.h"
#include <unordered_map>
#include <set>
#include <mutex>
#include <atomic>
#include <string>

using websocketpp_server = websocketpp::server<websocketpp::config::asio>;
using connection_hdl = websocketpp::connection_hdl;
using message_ptr = websocketpp_server::message_ptr;

class WebSocketServer {
private:
    websocketpp_server m_server;
    std::unordered_map<std::string, std::set<connection_hdl, std::owner_less<connection_hdl>>> m_subscriptions;
    std::set<connection_hdl, std::owner_less<connection_hdl>> m_connections;
    std::mutex m_mutex;
    std::atomic<uint64_t> m_connection_count{0};
    PerformanceMonitor& m_performance_monitor;

    // WebSocket event handlers
    void on_open(connection_hdl hdl);
    void on_close(connection_hdl hdl);
    void on_message(connection_hdl hdl, message_ptr msg);

public:
    explicit WebSocketServer(PerformanceMonitor& monitor);
    ~WebSocketServer();

    // Prevent copying and assignment
    WebSocketServer(const WebSocketServer&) = delete;
    WebSocketServer& operator=(const WebSocketServer&) = delete;
    WebSocketServer(WebSocketServer&&) = delete;
    WebSocketServer& operator=(WebSocketServer&&) = delete;

    void start(uint16_t port);
    void stop();
    void broadcast(const std::string& symbol, const std::string& message);
    void handle_subscription(connection_hdl hdl, const std::string& symbol);
    void remove_connection(connection_hdl hdl);
    
    uint64_t get_total_connections() const { return m_connection_count; }
}; 