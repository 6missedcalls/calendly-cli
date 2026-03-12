#include "core/filter.h"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <curl/curl.h>

namespace {

const std::string CALENDLY_BASE_URL = "https://api.calendly.com/";

// RAII guard for a temporary CURL handle used in URL-encoding
struct CurlHandleGuard {
    CURL* handle;

    CurlHandleGuard() : handle(curl_easy_init()) {}

    ~CurlHandleGuard() {
        if (handle != nullptr) {
            curl_easy_cleanup(handle);
        }
    }

    CurlHandleGuard(const CurlHandleGuard&) = delete;
    CurlHandleGuard& operator=(const CurlHandleGuard&) = delete;
};

// RAII guard for curl_easy_escape output
struct CurlStringGuard {
    char* str;

    explicit CurlStringGuard(char* s) : str(s) {}

    ~CurlStringGuard() {
        if (str != nullptr) {
            curl_free(str);
        }
    }

    CurlStringGuard(const CurlStringGuard&) = delete;
    CurlStringGuard& operator=(const CurlStringGuard&) = delete;
};

// Parse "YYYY-MM-DDTHH:MM:SSZ" into time_t (UTC)
std::optional<std::time_t> parse_iso8601(const std::string& iso8601) {
    std::tm tm_val = {};
    std::istringstream ss(iso8601);
    ss >> std::get_time(&tm_val, "%Y-%m-%dT%H:%M:%S");

    if (ss.fail()) {
        return std::nullopt;
    }

    // timegm interprets tm as UTC (unlike mktime which uses local time)
    const auto result = timegm(&tm_val);
    if (result == static_cast<std::time_t>(-1)) {
        return std::nullopt;
    }

    return result;
}

} // namespace

std::string extract_uuid(const std::string& uri) {
    const auto pos = uri.rfind('/');
    if (pos == std::string::npos) {
        return uri;
    }
    return uri.substr(pos + 1);
}

std::string build_uri(const std::string& resource_type, const std::string& uuid) {
    return CALENDLY_BASE_URL + resource_type + "/" + uuid;
}

bool is_valid_timezone(const std::string& tz) {
    if (tz.empty()) {
        return false;
    }

    if (tz == "UTC") {
        return true;
    }

    return tz.find('/') != std::string::npos;
}

std::string build_query_string(const std::map<std::string, std::string>& params) {
    if (params.empty()) {
        return "";
    }

    CurlHandleGuard guard;
    if (guard.handle == nullptr) {
        throw std::runtime_error("Failed to initialize curl handle for URL encoding");
    }

    std::ostringstream oss;
    oss << "?";

    bool first = true;
    for (const auto& [key, value] : params) {
        if (!first) {
            oss << "&";
        }
        first = false;

        CurlStringGuard encoded_key(
            curl_easy_escape(guard.handle, key.c_str(), static_cast<int>(key.size()))
        );
        CurlStringGuard encoded_value(
            curl_easy_escape(guard.handle, value.c_str(), static_cast<int>(value.size()))
        );

        if (encoded_key.str == nullptr || encoded_value.str == nullptr) {
            throw std::runtime_error("Failed to URL-encode query parameter");
        }

        oss << encoded_key.str << "=" << encoded_value.str;
    }

    return oss.str();
}

std::string format_relative_time(const std::string& iso8601) {
    const auto parsed = parse_iso8601(iso8601);
    if (!parsed.has_value()) {
        return iso8601;
    }

    const auto now = std::time(nullptr);
    const auto diff_seconds = static_cast<long long>(now) - static_cast<long long>(*parsed);
    const auto abs_diff = std::abs(diff_seconds);
    const bool is_future = diff_seconds < 0;

    constexpr long long SECONDS_PER_MINUTE = 60;
    constexpr long long SECONDS_PER_HOUR = 3600;
    constexpr long long SECONDS_PER_DAY = 86400;
    constexpr long long SECONDS_PER_WEEK = 604800;

    if (abs_diff < SECONDS_PER_MINUTE) {
        return "just now";
    }

    if (abs_diff < SECONDS_PER_HOUR) {
        const auto minutes = abs_diff / SECONDS_PER_MINUTE;
        const auto label = std::to_string(minutes) + (minutes == 1 ? " minute" : " minutes");
        return is_future ? "in " + label : label + " ago";
    }

    if (abs_diff < SECONDS_PER_DAY) {
        const auto hours = abs_diff / SECONDS_PER_HOUR;
        const auto label = std::to_string(hours) + (hours == 1 ? " hour" : " hours");
        return is_future ? "in " + label : label + " ago";
    }

    if (abs_diff < SECONDS_PER_WEEK) {
        const auto days = abs_diff / SECONDS_PER_DAY;
        const auto label = std::to_string(days) + (days == 1 ? " day" : " days");
        return is_future ? "in " + label : label + " ago";
    }

    // Fallback: absolute date format
    std::tm tm_val = {};
    const auto ts = *parsed;
    gmtime_r(&ts, &tm_val);

    std::ostringstream oss;
    oss << std::put_time(&tm_val, "%Y-%m-%d %H:%M");
    return oss.str();
}

int validate_count(int count) {
    return std::clamp(count, 1, 100);
}

std::string validate_sort(const std::string& sort) {
    const auto colon_pos = sort.find(':');
    if (colon_pos == std::string::npos || colon_pos == 0 || colon_pos == sort.size() - 1) {
        throw std::invalid_argument(
            "Invalid sort format: '" + sort + "'. Expected 'field:direction' (e.g., 'created_at:asc')"
        );
    }

    const auto direction = sort.substr(colon_pos + 1);
    if (direction != "asc" && direction != "desc") {
        throw std::invalid_argument(
            "Invalid sort direction: '" + direction + "'. Must be 'asc' or 'desc'"
        );
    }

    return sort;
}

std::string short_uuid(const std::string& uuid) {
    constexpr std::size_t SHORT_LENGTH = 8;
    if (uuid.size() <= SHORT_LENGTH) {
        return uuid;
    }
    return uuid.substr(0, SHORT_LENGTH);
}

std::string strip_base_url(const std::string& uri) {
    if (uri.rfind(CALENDLY_BASE_URL, 0) == 0) {
        return uri.substr(CALENDLY_BASE_URL.size());
    }
    return uri;
}
