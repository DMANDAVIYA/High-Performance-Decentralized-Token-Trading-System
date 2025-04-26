#include "utility_manager.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <random>
#include <algorithm>
#include <cctype>

void UtilityManager::HandleExitSignal(const int signal) {
    std::cout << "Received signal: " << signal << std::endl;
    exit(signal);
}

void UtilityManager::DisplayJsonResponse(const std::string& response) {
    std::cout << response << std::endl;
}

std::string UtilityManager::DisplayFormattedTimestamp(const int64_t& timestamp_ms) {
    auto time = std::chrono::system_clock::from_time_t(timestamp_ms / 1000);
    auto ms = timestamp_ms % 1000;
    auto time_t = std::chrono::system_clock::to_time_t(time);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << "." << std::setfill('0') << std::setw(3) << ms;
    return ss.str();
}

void UtilityManager::DisplayCurrentPositionsJson(const std::string& response) {
    std::cout << response << std::endl;
}

bool UtilityManager::IsParseJsonGood(const std::string& response, Json::Value& json_data) {
    Json::Reader reader;
    return reader.parse(response, json_data);
}

void UtilityManager::DisplayOrderBookJson(const std::string& response) {
    std::cout << response << std::endl;
}

std::string UtilityManager::get_current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << "." << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

std::string UtilityManager::format_price(double price) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << price;
    return ss.str();
}

std::string UtilityManager::format_amount(double amount) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(8) << amount;
    return ss.str();
}

bool UtilityManager::validate_symbol(const std::string& symbol) {
    return !symbol.empty() && symbol.length() <= 32;
}

bool UtilityManager::validate_price(double price) {
    return price > 0.0;
}

bool UtilityManager::validate_amount(double amount) {
    return amount > 0.0;
}

std::vector<std::string> UtilityManager::split_string(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

std::string UtilityManager::trim_string(const std::string& str) {
    auto start = std::find_if_not(str.begin(), str.end(), [](unsigned char c) { return std::isspace(c); });
    auto end = std::find_if_not(str.rbegin(), str.rend(), [](unsigned char c) { return std::isspace(c); }).base();
    return (start < end) ? std::string(start, end) : std::string();
}

double UtilityManager::calculate_pnl(double entry_price, double exit_price, double amount, bool is_long) {
    return is_long ? (exit_price - entry_price) * amount : (entry_price - exit_price) * amount;
}

std::string UtilityManager::generate_order_id() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    static const char* hex_chars = "0123456789ABCDEF";
    
    std::string uuid;
    uuid.reserve(36);
    
    for (int i = 0; i < 32; i++) {
        uuid += hex_chars[dis(gen)];
        if (i == 7 || i == 11 || i == 15 || i == 19) {
            uuid += '-';
        }
    }
    
    return uuid;
}
