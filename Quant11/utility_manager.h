#pragma once
#include <string>
#include <vector>
#include <chrono>

class UtilityManager
{
  public:
    static void HandleExitSignal(const int signal);
    static void DisplayJsonResponse(const std::string& response);

    static std::string DisplayFormattedTimestamp(const int64_t& timestamp_ms);

    static void DisplayCurrentPositionsJson(const std::string& response);
    static bool IsParseJsonGood(const std::string& response, Json::Value& json_data);

    static void DisplayOrderBookJson(const std::string& response);

    static std::string get_current_timestamp();
    static std::string format_price(double price);
    static std::string format_amount(double amount);
    static bool validate_symbol(const std::string& symbol);
    static bool validate_price(double price);
    static bool validate_amount(double amount);
    static std::vector<std::string> split_string(const std::string& str, char delimiter);
    static std::string trim_string(const std::string& str);
    static double calculate_pnl(double entry_price, double exit_price, double amount, bool is_long);
    static std::string generate_order_id();
};
