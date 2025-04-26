#include "api_manager.h"
#include <curl/curl.h>
#include <json/json.h>
#include <sstream>
#include <iomanip>
#include <nlohmann/json.hpp>
#include <drogon/HttpClient.h>
#include <chrono>
#include <iostream>

using json = nlohmann::json;

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

ApiManager::ApiManager(const std::string& base_url) : m_base_url(base_url) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

ApiManager::~ApiManager() {
    curl_global_cleanup();
}

std::string ApiManager::GetFormattedTimestamp() {
    const auto now = std::chrono::system_clock::now();
    const auto now_time = std::chrono::system_clock::to_time_t(now);
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000;

    struct tm timeinfo;
    char timestamp[20];
    localtime_r(&now_time, &timeinfo);
    std::strftime(timestamp, sizeof(timestamp), "%H:%M:%S", &timeinfo);

    std::stringstream ss;
    ss << timestamp << "." << std::setfill('0') << std::setw(3) << ms;
    return ss.str();
}

std::string ApiManager::SendRequest(const std::string& endpoint, const std::string& method, const std::string& data) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        return "Error initializing CURL";
    }

    std::string url = m_base_url + endpoint;
    std::string response;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    if (method == "POST") {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
    }

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        response = "Error: " + std::string(curl_easy_strerror(res));
    }

    curl_easy_cleanup(curl);
    return response;
}

std::string ApiManager::PlaceOrder(const std::string& instrument_name, const std::string& side, double amount, double price) {
    json order;
    order["jsonrpc"] = "2.0";
    order["method"] = "private/buy";
    order["params"]["instrument_name"] = instrument_name;
    order["params"]["amount"] = amount;
    order["params"]["price"] = price;
    order["params"]["type"] = "limit";
    order["id"] = 1;

    return SendRequest("/api/v2/private/buy", "POST", order.dump());
}

std::string ApiManager::CancelOrder(const std::string& order_id) {
    json cancel;
    cancel["jsonrpc"] = "2.0";
    cancel["method"] = "private/cancel";
    cancel["params"]["order_id"] = order_id;
    cancel["id"] = 2;

    return SendRequest("/api/v2/private/cancel", "POST", cancel.dump());
}

std::string ApiManager::GetOrderBook(const std::string& instrument_name) {
    json request;
    request["jsonrpc"] = "2.0";
    request["method"] = "public/get_orderbook";
    request["params"]["instrument_name"] = instrument_name;
    request["id"] = 3;

    return SendRequest("/api/v2/public/get_orderbook", "POST", request.dump());
}

std::string ApiManager::GetAccountSummary() {
    json request;
    request["jsonrpc"] = "2.0";
    request["method"] = "private/get_account_summary";
    request["id"] = 4;

    return SendRequest("/api/v2/private/get_account_summary", "POST", request.dump());
}

void ApiManager::modify_order(const std::string& order_id, double new_price, double new_amount) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    json request;
    request["jsonrpc"] = "2.0";
    request["method"] = "private/edit";
    request["params"]["order_id"] = order_id;
    request["params"]["price"] = new_price;
    request["params"]["amount"] = new_amount;
    request["id"] = 1;

    auto client = drogon::HttpClient::newHttpClient("https://test.deribit.com");
    auto req = drogon::HttpRequest::newHttpRequest();
    req->setMethod(drogon::Post);
    req->setPath("/api/v2/private/edit");
    req->setBody(request.dump());
    req->addHeader("Authorization", "Bearer " + m_token_manager.get_access_token());

    try {
        auto resp = client->sendRequest(req);
        auto end_time = std::chrono::high_resolution_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
        
        if (resp->statusCode() == drogon::k200OK) {
            json response;
            if (resp->body().empty()) {
                response = json::parse("{}");
            } else {
                response = json::parse(resp->body());
            }
            if (response.isMember("result")) {
                std::cout << "Order modified successfully. Latency: " << latency << " μs" << std::endl;
                return;
            }
        }
        std::cerr << "Failed to modify order. Status: " << resp->statusCode() 
                  << ", Body: " << resp->body() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error modifying order: " << e.what() << std::endl;
    }
}

