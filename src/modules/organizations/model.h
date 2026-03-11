#pragma once
#include <optional>
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct OrganizationInvitation {
    std::string uri;
    std::string organization;    // Org URI
    std::string email;
    std::string status;          // "pending", "accepted", "declined"
    std::string created_at;
    std::string updated_at;
    std::optional<std::string> last_sent_at;
    std::optional<std::string> user;  // User URI, only when status == "accepted"
};

inline void from_json(const json& j, OrganizationInvitation& oi) {
    oi.uri = j.value("uri", "");
    oi.organization = j.value("organization", "");
    oi.email = j.value("email", "");
    oi.status = j.value("status", "");
    oi.created_at = j.value("created_at", "");
    oi.updated_at = j.value("updated_at", "");
    if (j.contains("last_sent_at") && !j["last_sent_at"].is_null()) oi.last_sent_at = j["last_sent_at"].get<std::string>();
    if (j.contains("user") && !j["user"].is_null()) oi.user = j["user"].get<std::string>();
}
