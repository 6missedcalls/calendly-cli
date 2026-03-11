#pragma once
#include <optional>
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct User {
    std::string uri;                              // https://api.calendly.com/users/AAAA...
    std::string name;
    std::string slug;
    std::string email;
    std::string scheduling_url;
    std::string timezone;                         // IANA timezone (e.g. "America/New_York")
    std::string time_notation;                    // "12h" or "24h"
    std::optional<std::string> avatar_url;        // Nullable
    std::string created_at;
    std::string updated_at;
    std::string current_organization;             // Full org URI
    std::string resource_type;                    // Always "User"
    std::string locale;                           // e.g. "en"
};

inline void from_json(const json& j, User& u) {
    u.uri = j.value("uri", "");
    u.name = j.value("name", "");
    u.slug = j.value("slug", "");
    u.email = j.value("email", "");
    u.scheduling_url = j.value("scheduling_url", "");
    u.timezone = j.value("timezone", "");
    u.time_notation = j.value("time_notation", "12h");
    if (j.contains("avatar_url") && !j["avatar_url"].is_null()) {
        u.avatar_url = j["avatar_url"].get<std::string>();
    }
    u.created_at = j.value("created_at", "");
    u.updated_at = j.value("updated_at", "");
    u.current_organization = j.value("current_organization", "");
    u.resource_type = j.value("resource_type", "User");
    u.locale = j.value("locale", "en");
}
