#include "api_credentials.h"
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <stdexcept>

namespace {
    const char* API_KEY_ENV = "DERIBIT_API_KEY";
    const char* API_SECRET_ENV = "DERIBIT_API_SECRET";
    const char* API_CREDENTIALS_FILE = "api.env";

    std::string read_file_contents(const std::string& file_path) {
        std::ifstream file(file_path);
        if (!file.is_open()) {
            return "";
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    std::string get_env_variable(const char* env_var) {
        const char* value = std::getenv(env_var);
        return value ? value : "";
    }

    bool validate_api_key(const std::string& key) {
        return !key.empty() && key.length() >= 8;
    }

    bool validate_api_secret(const std::string& secret) {
        return !secret.empty() && secret.length() >= 32;
    }
}

ApiCredentials::ApiCredentials() {
    // Try environment variables first
    m_api_key = get_env_variable(API_KEY_ENV);
    m_api_secret = get_env_variable(API_SECRET_ENV);

    // If not found in environment, try config file
    if (m_api_key.empty() || m_api_secret.empty()) {
        std::string file_contents = read_file_contents(API_CREDENTIALS_FILE);
        if (!file_contents.empty()) {
            std::istringstream iss(file_contents);
            std::string line;
            while (std::getline(iss, line)) {
                size_t pos = line.find('=');
                if (pos != std::string::npos) {
                    std::string key = line.substr(0, pos);
                    std::string value = line.substr(pos + 1);
                    if (key == "API_KEY") m_api_key = value;
                    else if (key == "SECRET_KEY") m_api_secret = value;
                }
            }
        }
    }

    // Validate credentials
    if (!validate_api_key(m_api_key) || !validate_api_secret(m_api_secret)) {
        throw std::runtime_error("Invalid API credentials. Please check your environment variables or config file.");
    }
}

std::string ApiCredentials::get_api_key() const {
    return m_api_key;
}

std::string ApiCredentials::get_api_secret() const {
    return m_api_secret;
}
