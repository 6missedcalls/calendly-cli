#include "core/rest_client.h"
#include <string>
#include <curl/curl.h>
#include "core/auth.h"
#include "core/error.h"
#include "core/http_client.h"
#include "core/output.h"

using json = nlohmann::json;

static const std::string BASE_URL = "https://api.calendly.com/";

// RAII wrapper for a temporary CURL handle used solely for URL encoding.
class CurlGuard {
public:
    CurlGuard() : handle_(curl_easy_init()) {}
    ~CurlGuard() {
        if (handle_) {
            curl_easy_cleanup(handle_);
        }
    }

    CurlGuard(const CurlGuard&) = delete;
    CurlGuard& operator=(const CurlGuard&) = delete;

    CURL* get() const noexcept { return handle_; }

private:
    CURL* handle_;
};

std::string build_url(
    const std::string& path,
    const std::map<std::string, std::string>& query_params) {

    auto url = BASE_URL + path;

    if (query_params.empty()) {
        return url;
    }

    CurlGuard curl;
    if (!curl.get()) {
        throw CalendlyError(ErrorKind::Internal, "Failed to initialize CURL for URL encoding");
    }

    url += '?';
    bool first = true;

    for (const auto& [key, value] : query_params) {
        if (!first) {
            url += '&';
        }
        first = false;

        char* escaped = curl_easy_escape(curl.get(), value.c_str(), static_cast<int>(value.size()));
        if (!escaped) {
            throw CalendlyError(ErrorKind::Internal, "Failed to URL-encode parameter: " + key);
        }
        url += key + '=' + std::string(escaped);
        curl_free(escaped);
    }

    return url;
}

static json parse_json_response(const std::string& body) {
    try {
        return json::parse(body);
    } catch (const json::parse_error& e) {
        throw CalendlyError(ErrorKind::Internal,
            std::string("Failed to parse JSON response: ") + e.what());
    }
}

json rest_get(
    const std::string& path,
    const std::map<std::string, std::string>& query_params) {

    return with_retry([&]() {
        auto url = build_url(path, query_params);
        print_verbose(">> GET " + url);

        auto headers = get_auth_headers();
        headers.emplace_back("Content-Type", "application/json");

        HttpClient client;
        auto response = client.get(url, headers);
        print_verbose("<< " + std::to_string(response.status_code)
                      + " (" + std::to_string(response.body.size()) + " bytes)");

        // Curl transport error (no HTTP response received)
        if (response.status_code == 0 && !response.error_message.empty()) {
            throw CalendlyError(ErrorKind::Network, response.error_message);
        }

        // For 429, pass rate-limit reset header to check_http_status via retry_after
        if (response.status_code == 429 && response.rate_limit.reset_seconds > 0) {
            throw CalendlyError(ErrorKind::RateLimit,
                "Rate limited by Calendly API",
                response.rate_limit.reset_seconds);
        }

        check_http_status(response.status_code, response.body);
        return parse_json_response(response.body);
    });
}

json rest_post(
    const std::string& path,
    const json& body) {

    return with_retry([&]() {
        auto url = build_url(path);
        print_verbose(">> POST " + url);

        auto headers = get_auth_headers();
        headers.emplace_back("Content-Type", "application/json");

        HttpClient client;
        auto response = client.post(url, body.dump(), headers);
        print_verbose("<< " + std::to_string(response.status_code)
                      + " (" + std::to_string(response.body.size()) + " bytes)");

        if (response.status_code == 0 && !response.error_message.empty()) {
            throw CalendlyError(ErrorKind::Network, response.error_message);
        }

        if (response.status_code == 429 && response.rate_limit.reset_seconds > 0) {
            throw CalendlyError(ErrorKind::RateLimit,
                "Rate limited by Calendly API",
                response.rate_limit.reset_seconds);
        }

        check_http_status(response.status_code, response.body);
        return parse_json_response(response.body);
    });
}
