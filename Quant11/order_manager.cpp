#include "order_manager.h"
#include "utility_manager.h"
#include "token_manager.h"
#include <drogon/HttpClient.h>
#include <json/json.h>
#include <iostream>
#include <future>
#include <algorithm>

bool OrderManager::PlaceOrder(const OrderParams& params, const std::string& side, std::string& response) const
{
    if (!RefreshTokenIfNeeded())
    {
        return false;
    }

    const std::string access_token = m_token_manager.GetAccessToken();
    const auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(drogon::Post);
    req->setPath("/api/v2/private/" + side);

    Json::Value json_params;
    json_params["instrument_name"] = params.instrument_name;
    json_params["amount"] = params.amount;
    json_params["price"] = params.price;
    json_params["label"] = params.label;
    json_params["type"] = GetOrderTypeString(params.type);
    json_params["time_in_force"] = params.time_in_force;

    Json::Value json_request;
    json_request["jsonrpc"] = "2.0";
    json_request["id"] = 9929;
    json_request["method"] = "private/" + side;
    json_request["params"] = json_params;

    Json::StreamWriterBuilder writer;
    const std::string request_body = Json::writeString(writer, json_request);

    req->setBody(request_body);
    req->addHeader("Authorization", "Bearer " + access_token);
    req->addHeader("Content-Type", "application/json");

    std::promise<bool> promise;
    auto future = promise.get_future();

    m_client->sendRequest(
        req,
        [&](const drogon::ReqResult& result, const drogon::HttpResponsePtr& http_response)
        {
            if (result == drogon::ReqResult::Ok && http_response->getStatusCode() == drogon::k200OK)
            {
                response = http_response->body();
                UtilityManager::DisplayJsonResponse(response);
                promise.set_value(true);
            }
            else
            {
                std::cerr << "Failed to place order.\n";
                response = "Failed to place order";
                promise.set_value(false);
            }
        });

    return future.get();
}

void OrderManager::add_order(const Order& order) {
    std::lock_guard<std::mutex> lock(m_orders_mutex);
    m_orders[order.order_id] = order;
}

void OrderManager::update_order_status(const std::string& order_id, const std::string& status) {
    std::lock_guard<std::mutex> lock(m_orders_mutex);
    auto it = m_orders.find(order_id);
    if (it != m_orders.end()) {
        it->second.status = status;
    }
}

void OrderManager::remove_order(const std::string& order_id) {
    std::lock_guard<std::mutex> lock(m_orders_mutex);
    m_orders.erase(order_id);
}

Order OrderManager::get_order(const std::string& order_id) const {
    std::lock_guard<std::mutex> lock(m_orders_mutex);
    auto it = m_orders.find(order_id);
    if (it != m_orders.end()) {
        return it->second;
    }
    return Order{};
}

std::vector<Order> OrderManager::get_all_orders() const {
    std::lock_guard<std::mutex> lock(m_orders_mutex);
    std::vector<Order> orders;
    orders.reserve(m_orders.size());
    for (const auto& pair : m_orders) {
        orders.push_back(pair.second);
    }
    return orders;
}

std::vector<Order> OrderManager::get_orders_by_symbol(const std::string& symbol) const {
    std::lock_guard<std::mutex> lock(m_orders_mutex);
    std::vector<Order> orders;
    for (const auto& pair : m_orders) {
        if (pair.second.symbol == symbol) {
            orders.push_back(pair.second);
        }
    }
    return orders;
}

std::vector<Order> OrderManager::get_orders_by_status(const std::string& status) const {
    std::lock_guard<std::mutex> lock(m_orders_mutex);
    std::vector<Order> orders;
    for (const auto& pair : m_orders) {
        if (pair.second.status == status) {
            orders.push_back(pair.second);
        }
    }
    return orders;
}