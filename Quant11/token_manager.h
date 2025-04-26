#pragma once

#include <chrono>
#include <string>

class TokenManager
{
  private:
    std::string m_access_token;
    std::string m_refresh_token;
    std::chrono::system_clock::time_point m_token_expiry;

    bool read_tokens_from_file();
    bool is_token_expired() const;
    bool refresh_token();

  public:
    TokenManager();
    std::string get_access_token();
    void update_tokens(const std::string& access_token, const std::string& refresh_token);
};
