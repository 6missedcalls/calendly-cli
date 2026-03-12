#pragma once
#include <string>
#include <utility>
#include <vector>

// Resolution order:
// 1. CALENDLY_API_TOKEN environment variable
// 2. token field in ~/.config/calendly/config.toml
// 3. Error with instructions
std::string get_api_token();

// Returns auth headers for Calendly API requests.
// Calendly uses Bearer token auth.
std::vector<std::pair<std::string, std::string>> get_auth_headers();
