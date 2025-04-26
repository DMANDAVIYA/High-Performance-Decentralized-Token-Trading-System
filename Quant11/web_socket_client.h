#pragma once
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_client.hpp>
#include <functional>
#include <string>
#include <mutex>
#include <queue>
#include <condition_variable>

typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
typedef websocketpp::config::asio_tls_client::message_type::ptr message_ptr;
typedef websocketpp::connection_hdl connection_hdl;

class WebSocketClient {
private:
    client m_client;
    connection_hdl m_connection;
    std::mutex m_connection_mutex;
    std::queue<std::string> m_message_queue;
    std::mutex m_queue_mutex;
    std::condition_variable m_queue_cv;
    bool m_connected;
    std::mutex m_connected_mutex;

    void on_message(connection_hdl hdl, message_ptr msg);
    void on_open(connection_hdl hdl);
    void on_close(connection_hdl hdl);
    void on_fail(connection_hdl hdl);

public:
    WebSocketClient();
    ~WebSocketClient();

    bool connect(const std::string& uri);
    void disconnect();
    void send_message(const std::string& message);
    std::string receive_message();
    bool is_connected() const;
};