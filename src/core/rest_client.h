#pragma once
#include <map>
#include <string>
#include <nlohmann/json.hpp>

// Build full Calendly API URL from path and optional query parameters.
// Base: https://api.calendly.com/
// Query parameter values are URL-encoded.
std::string build_url(
    const std::string& path,
    const std::map<std::string, std::string>& query_params = {});

// Execute REST calls — each handles auth, error checking, JSON parsing.
// All functions automatically retry on 429 rate limit errors.

nlohmann::json rest_get(
    const std::string& path,
    const std::map<std::string, std::string>& query_params = {});

nlohmann::json rest_post(
    const std::string& path,
    const nlohmann::json& body = nlohmann::json::object());
