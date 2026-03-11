#pragma once

#include <chrono>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>

// Forward-declare print_verbose to avoid circular dependency with output.h
void print_verbose(const std::string& message);

enum class ErrorKind {
    Network,
    Auth,
    Forbidden,
    NotFound,
    RateLimit,
    Api,
    Validation,
    Internal
};

class CalendlyError : public std::runtime_error {
public:
    CalendlyError(int status_code, const std::string& title, const std::string& message);
    CalendlyError(ErrorKind kind, const std::string& message);
    CalendlyError(ErrorKind kind, const std::string& message, int retry_after);

    int status_code() const noexcept;
    const std::string& title() const noexcept;
    const std::string& api_message() const noexcept;
    ErrorKind kind() const noexcept;
    std::optional<int> retry_after() const noexcept;

private:
    int status_code_;
    std::string title_;
    std::string api_message_;
    ErrorKind kind_;
    std::optional<int> retry_after_;
};

// Parse Calendly error response format: {"title": "...", "message": "..."}
// Throws CalendlyError if status_code indicates an error (4xx/5xx)
void check_http_status(long status_code, const std::string& response_body);

// Retry with exponential backoff for 429 responses.
// Uses X-RateLimit-Reset from CalendlyError's retry_after.
template<typename Fn>
auto with_retry(Fn&& fn, int max_retries = 3) -> decltype(fn()) {
    for (int attempt = 0; attempt <= max_retries; ++attempt) {
        try {
            return fn();
        } catch (const CalendlyError& e) {
            if (e.kind() != ErrorKind::RateLimit || attempt == max_retries) {
                throw;
            }
            int wait_seconds = e.retry_after().value_or(1 << attempt);
            std::this_thread::sleep_for(std::chrono::seconds(wait_seconds));
            print_verbose("retrying (" + std::to_string(attempt + 1) + "/"
                          + std::to_string(max_retries) + ") after "
                          + std::to_string(wait_seconds) + "s");
        }
    }
    throw CalendlyError(ErrorKind::Internal, "Retry loop exited unexpectedly");
}

// Format error for human-readable display
std::string format_error(const CalendlyError& e);
