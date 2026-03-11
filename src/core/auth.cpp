#include "core/auth.h"
#include "core/config.h"
#include "core/error.h"
#include <cstdlib>

std::string get_api_token() {
    const char* env_token = std::getenv("CALENDLY_API_TOKEN");
    if (env_token != nullptr && env_token[0] != '\0') {
        return std::string(env_token);
    }

    if (config_exists()) {
        CalendlyConfig cfg = load_config();
        if (cfg.token.has_value() && !cfg.token->empty()) {
            return cfg.token.value();
        }
    }

    throw CalendlyError(
        ErrorKind::Auth,
        "No API token found. Set CALENDLY_API_TOKEN environment variable "
        "or add token to ~/.config/calendly/config.toml"
    );
}

bool validate_api_token(const std::string& token) {
    return !token.empty();
}

bool has_api_token() {
    const char* env_token = std::getenv("CALENDLY_API_TOKEN");
    if (env_token != nullptr && env_token[0] != '\0') {
        return true;
    }

    try {
        if (config_exists()) {
            CalendlyConfig cfg = load_config();
            return cfg.token.has_value() && !cfg.token->empty();
        }
    } catch (...) {
        return false;
    }

    return false;
}

std::vector<std::pair<std::string, std::string>> get_auth_headers() {
    return {{"Authorization", "Bearer " + get_api_token()}};
}
