#pragma once
#include <string>
#include <memory>
#include "api_credentials.h"
#include "token_manager.h"

class ApiManager {
private:
    std::unique_ptr<ApiCredentials> m_credentials;
    std::unique_ptr<TokenManager> m_token_manager;
    std::string m_base_url;

public:
    ApiManager();
    std::string place_order(const std::string& symbol, const std::string& side, 
                          double price, double amount);
    std::string cancel_order(const std::string& order_id);
    std::string get_order_status(const std::string& order_id);
    std::string get_account_balance();
};