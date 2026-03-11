#include "http_client.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace {

struct WriteContext {
    std::string* buffer;
};

struct HeaderContext {
    std::string* buffer;
};

auto write_callback(char* buffer, size_t size, size_t nitems, void* userdata) -> size_t {
    auto* context = static_cast<WriteContext*>(userdata);
    const auto total_size = size * nitems;
    context->buffer->append(buffer, total_size);
    return total_size;
}

auto header_callback(char* buffer, size_t size, size_t nitems, void* userdata) -> size_t {
    auto* context = static_cast<HeaderContext*>(userdata);
    const auto total_size = size * nitems;
    context->buffer->append(buffer, total_size);
    return total_size;
}

auto to_lower(const std::string& input) -> std::string {
    auto result = input;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char ch) { return std::tolower(ch); });
    return result;
}

auto extract_header_value(const std::string& raw_headers, const std::string& header_name) -> std::string {
    const auto lower_headers = to_lower(raw_headers);
    const auto lower_name = to_lower(header_name);

    auto pos = lower_headers.find(lower_name + ":");
    if (pos == std::string::npos) {
        return "";
    }

    pos = pos + lower_name.size() + 1;

    while (pos < raw_headers.size() && raw_headers[pos] == ' ') {
        ++pos;
    }

    auto end = raw_headers.find("\r\n", pos);
    if (end == std::string::npos) {
        end = raw_headers.find('\n', pos);
    }
    if (end == std::string::npos) {
        end = raw_headers.size();
    }

    return raw_headers.substr(pos, end - pos);
}

auto parse_rate_limit_headers(const std::string& raw_headers) -> RateLimitInfo {
    auto info = RateLimitInfo{};

    const auto limit_str = extract_header_value(raw_headers, "x-ratelimit-limit");
    const auto remaining_str = extract_header_value(raw_headers, "x-ratelimit-remaining");
    const auto reset_str = extract_header_value(raw_headers, "x-ratelimit-reset");

    try {
        if (!limit_str.empty()) {
            info.limit = std::stoi(limit_str);
        }
    } catch (const std::exception&) {
        // Non-numeric header value; keep default
    }

    try {
        if (!remaining_str.empty()) {
            info.remaining = std::stoi(remaining_str);
        }
    } catch (const std::exception&) {
        // Non-numeric header value; keep default
    }

    try {
        if (!reset_str.empty()) {
            info.reset_seconds = std::stoi(reset_str);
        }
    } catch (const std::exception&) {
        // Non-numeric header value; keep default
    }

    return info;
}

} // namespace

HttpClient::HttpClient() {
    curl_handle_ = curl_easy_init();
    if (curl_handle_ == nullptr) {
        throw std::runtime_error("Failed to initialize curl handle");
    }
}

HttpClient::~HttpClient() {
    if (curl_handle_ != nullptr) {
        curl_easy_cleanup(curl_handle_);
    }
}

HttpClient::HttpClient(HttpClient&& other) noexcept
    : curl_handle_(other.curl_handle_)
    , timeout_seconds_(other.timeout_seconds_) {
    other.curl_handle_ = nullptr;
}

HttpClient& HttpClient::operator=(HttpClient&& other) noexcept {
    if (this != &other) {
        if (curl_handle_ != nullptr) {
            curl_easy_cleanup(curl_handle_);
        }
        curl_handle_ = other.curl_handle_;
        timeout_seconds_ = other.timeout_seconds_;
        other.curl_handle_ = nullptr;
    }
    return *this;
}

HttpResponse HttpClient::get(const std::string& url, const std::vector<std::pair<std::string, std::string>>& headers) {
    return execute(url, "GET", "", headers);
}

HttpResponse HttpClient::post(const std::string& url, const std::string& body, const std::vector<std::pair<std::string, std::string>>& headers) {
    return execute(url, "POST", body, headers);
}

void HttpClient::set_timeout(long timeout_seconds) noexcept {
    timeout_seconds_ = timeout_seconds;
}

HttpResponse HttpClient::execute(const std::string& url, const std::string& method, const std::string& body, const std::vector<std::pair<std::string, std::string>>& headers) {
    if (curl_handle_ == nullptr) {
        return HttpResponse{0, "", "Curl handle is null", {}};
    }

    curl_easy_reset(curl_handle_);

    auto response_body = std::string{};
    auto response_headers = std::string{};
    auto write_ctx = WriteContext{&response_body};
    auto header_ctx = HeaderContext{&response_headers};

    curl_easy_setopt(curl_handle_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl_handle_, CURLOPT_CUSTOMREQUEST, method.c_str());
    curl_easy_setopt(curl_handle_, CURLOPT_TIMEOUT, timeout_seconds_);

    curl_easy_setopt(curl_handle_, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl_handle_, CURLOPT_WRITEDATA, &write_ctx);

    curl_easy_setopt(curl_handle_, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(curl_handle_, CURLOPT_HEADERDATA, &header_ctx);

    curl_easy_setopt(curl_handle_, CURLOPT_FOLLOWLOCATION, 0L);
    curl_easy_setopt(curl_handle_, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl_handle_, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl_handle_, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(curl_handle_, CURLOPT_USERAGENT, "calendly-cli/0.1.0");

    if (method == "POST" && !body.empty()) {
        curl_easy_setopt(curl_handle_, CURLOPT_POSTFIELDS, body.c_str());
    }

    struct curl_slist* header_list = nullptr;
    for (const auto& [key, value] : headers) {
        const auto header_string = key + ": " + value;
        header_list = curl_slist_append(header_list, header_string.c_str());
    }
    if (header_list != nullptr) {
        curl_easy_setopt(curl_handle_, CURLOPT_HTTPHEADER, header_list);
    }

    const auto result = curl_easy_perform(curl_handle_);

    if (header_list != nullptr) {
        curl_slist_free_all(header_list);
    }

    auto response = HttpResponse{};

    if (result != CURLE_OK) {
        response.error_message = curl_easy_strerror(result);
        return response;
    }

    long status_code = 0;
    curl_easy_getinfo(curl_handle_, CURLINFO_RESPONSE_CODE, &status_code);

    response.status_code = status_code;
    response.body = std::move(response_body);
    response.rate_limit = parse_rate_limit_headers(response_headers);

    return response;
}
