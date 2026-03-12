#include "core/error.h"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

// --- ErrorKind derivation from HTTP status code ---

static ErrorKind kind_from_status(int status_code) {
    switch (status_code) {
        case 401: return ErrorKind::Auth;
        case 403: return ErrorKind::Forbidden;
        case 404: return ErrorKind::NotFound;
        case 422: return ErrorKind::Validation;
        case 429: return ErrorKind::RateLimit;
        default:
            if (status_code >= 500) { return ErrorKind::Internal; }
            return ErrorKind::Api;
    }
}

static std::string kind_label(ErrorKind kind) {
    switch (kind) {
        case ErrorKind::Network:    return "Network Error";
        case ErrorKind::Auth:       return "Authentication Error";
        case ErrorKind::Forbidden:  return "Forbidden";
        case ErrorKind::NotFound:   return "Not Found";
        case ErrorKind::RateLimit:  return "Rate Limited";
        case ErrorKind::Api:        return "API Error";
        case ErrorKind::Validation: return "Validation Error";
        case ErrorKind::Internal:   return "Internal Error";
    }
    return "Unknown Error";
}

// --- CalendlyError constructors ---

CalendlyError::CalendlyError(int status_code, const std::string& title, const std::string& message)
    : std::runtime_error(title + ": " + message)
    , status_code_(status_code)
    , title_(title)
    , api_message_(message)
    , kind_(kind_from_status(status_code))
    , retry_after_(std::nullopt) {}

CalendlyError::CalendlyError(ErrorKind kind, const std::string& message)
    : std::runtime_error(kind_label(kind) + ": " + message)
    , status_code_(0)
    , title_(kind_label(kind))
    , api_message_(message)
    , kind_(kind)
    , retry_after_(std::nullopt) {}

CalendlyError::CalendlyError(ErrorKind kind, const std::string& message, int retry_after)
    : std::runtime_error(kind_label(kind) + ": " + message)
    , status_code_(0)
    , title_(kind_label(kind))
    , api_message_(message)
    , kind_(kind)
    , retry_after_(retry_after) {}

// --- Accessors ---

int CalendlyError::status_code() const noexcept {
    return status_code_;
}

const std::string& CalendlyError::title() const noexcept {
    return title_;
}

const std::string& CalendlyError::api_message() const noexcept {
    return api_message_;
}

ErrorKind CalendlyError::kind() const noexcept {
    return kind_;
}

std::optional<int> CalendlyError::retry_after() const noexcept {
    return retry_after_;
}

// --- HTTP status checking ---

void check_http_status(long status_code, const std::string& response_body) {
    if (status_code >= 200 && status_code < 300) {
        return;
    }

    std::string title = "Error";
    std::string message = "HTTP " + std::to_string(status_code);
    int retry_after = 1;  // Sane default: 1 second minimum backoff

    try {
        auto body = json::parse(response_body);
        if (body.contains("title") && !body["title"].is_null()) {
            title = body["title"].get<std::string>();
        }
        if (body.contains("message") && !body["message"].is_null()) {
            message = body["message"].get<std::string>();
        }
    } catch (const json::parse_error&) {
        // Response is not valid JSON; use defaults
        if (!response_body.empty()) {
            message = response_body;
        }
    }

    auto error = CalendlyError(static_cast<int>(status_code), title, message);

    if (status_code == 429) {
        // For rate limit errors, attempt to extract retry_after from the response body.
        // The actual X-RateLimit-Reset header value is typically passed by the caller
        // through the RestClient layer. Here we provide a reasonable default.
        try {
            auto body = json::parse(response_body);
            if (body.contains("retry_after") && body["retry_after"].is_number()) {
                int server_val = body["retry_after"].get<int>();
                if (server_val > 0 && server_val <= 300) {
                    retry_after = server_val;
                }
            }
        } catch (...) {
            // Ignore errors; retry_after stays at default
        }
        throw CalendlyError(ErrorKind::RateLimit, message, retry_after);
    }

    throw error;
}

// --- Human-readable error formatting ---

std::string format_error(const CalendlyError& e) {
    std::string result;

    switch (e.kind()) {
        case ErrorKind::Auth:
            result = "Authentication failed: " + e.api_message()
                     + "\n\nSet your API token via:\n"
                     + "  export CALENDLY_API_TOKEN=<your-token>\n"
                     + "  or add 'token = \"<your-token>\"' to ~/.config/calendly/config.toml";
            break;

        case ErrorKind::Forbidden:
            result = "Access denied: " + e.api_message()
                     + "\n\nYour token may lack the required permissions for this operation.";
            break;

        case ErrorKind::NotFound:
            result = "Not found: " + e.api_message()
                     + "\n\nThe requested resource does not exist or has been deleted.";
            break;

        case ErrorKind::RateLimit:
            result = "Rate limited by Calendly API.";
            if (e.retry_after()) {
                result += " Retry after " + std::to_string(*e.retry_after()) + " seconds.";
            }
            break;

        case ErrorKind::Validation:
            result = "Validation error: " + e.api_message();
            break;

        case ErrorKind::Network:
            result = "Network error: " + e.api_message()
                     + "\n\nCheck your internet connection and try again.";
            break;

        case ErrorKind::Internal:
            result = "Calendly server error: " + e.api_message()
                     + "\n\nThis is likely a temporary issue. Please try again later.";
            break;

        case ErrorKind::Api:
            result = e.title() + ": " + e.api_message();
            if (e.status_code() > 0) {
                result += " (HTTP " + std::to_string(e.status_code()) + ")";
            }
            break;
    }

    return result;
}
