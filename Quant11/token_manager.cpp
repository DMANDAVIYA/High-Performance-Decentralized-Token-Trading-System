#include "token_manager.h"
#include <fstream>
#include <sstream>
#include <chrono>
#include <ctime>
#include <iomanip>

std::string read_file_contents(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

TokenManager::TokenManager() {
    load_tokens();
}

void TokenManager::load_tokens() {
    m_access_token = read_file_contents("access_token.txt");
    m_refresh_token = read_file_contents("refresh_token.txt");
}

bool TokenManager::is_token_expired() const {
    if (m_access_token.empty()) {
        return true;
    }
    return false;
}

std::string TokenManager::refresh_token(const std::string& api_key, const std::string& api_secret) {
    if (m_refresh_token.empty()) {
        return "";
    }
    return m_refresh_token;
}

std::string TokenManager::get_access_token() const {
    return m_access_token;
}

void TokenManager::update_tokens(const std::string& access_token, const std::string& refresh_token) {
    m_access_token = access_token;
    m_refresh_token = refresh_token;
    
    std::ofstream access_file("access_token.txt");
    if (access_file.is_open()) {
        access_file << access_token;
        access_file.close();
    }
    
    std::ofstream refresh_file("refresh_token.txt");
    if (refresh_file.is_open()) {
        refresh_file << refresh_token;
        refresh_file.close();
    }
}
