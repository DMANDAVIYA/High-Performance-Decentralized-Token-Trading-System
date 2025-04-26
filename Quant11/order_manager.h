#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <unordered_map>

struct Order {
    std::string order_id;
    std::string symbol;
    std::string side;
    double price;
    double amount;
    std::string status;
    std::string timestamp;
};

class OrderManager {
private:
    std::unordered_map<std::string, Order> m_orders;
    std::mutex m_orders_mutex;

public:
    void add_order(const Order& order);
    void update_order_status(const std::string& order_id, const std::string& status);
    void remove_order(const std::string& order_id);
    Order get_order(const std::string& order_id) const;
    std::vector<Order> get_all_orders() const;
    std::vector<Order> get_orders_by_symbol(const std::string& symbol) const;
    std::vector<Order> get_orders_by_status(const std::string& status) const;
};