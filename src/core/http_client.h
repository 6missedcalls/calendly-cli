#pragma once
#include <string>
#include <utility>
#include <vector>
#include <curl/curl.h>

struct RateLimitInfo {
    int limit = 0;
    int remaining = 0;
    int reset_seconds = 0;
};

struct HttpResponse {
    long status_code = 0;
    std::string body;
    std::string error_message;
    RateLimitInfo rate_limit;
};

class HttpClient {
public:
    HttpClient();
    ~HttpClient();

    HttpClient(const HttpClient&) = delete;
    HttpClient& operator=(const HttpClient&) = delete;
    HttpClient(HttpClient&& other) noexcept;
    HttpClient& operator=(HttpClient&& other) noexcept;

    HttpResponse get(const std::string& url, const std::vector<std::pair<std::string, std::string>>& headers);
    HttpResponse post(const std::string& url, const std::string& body, const std::vector<std::pair<std::string, std::string>>& headers);

    void set_timeout(long timeout_seconds) noexcept;

private:
    HttpResponse execute(const std::string& url, const std::string& method, const std::string& body, const std::vector<std::pair<std::string, std::string>>& headers);

    CURL* curl_handle_ = nullptr;
    long timeout_seconds_ = 30;
};
