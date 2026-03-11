#pragma once
#include <optional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct DateSetting {
    std::string type;            // "date_range"
    std::string start_date;      // "YYYY-MM-DD"
    std::string end_date;        // "YYYY-MM-DD"
};

inline json to_json_obj(const DateSetting& ds) {
    return {{"type", ds.type}, {"start_date", ds.start_date}, {"end_date", ds.end_date}};
}

struct OneOffLocation {
    std::string kind;
    std::optional<std::string> location;
    std::optional<std::string> additional_info;   // Sent as "additonal_info" (API typo, Resolution H-09)
};

inline json to_json_obj(const OneOffLocation& loc) {
    json j;
    j["kind"] = loc.kind;
    if (loc.location.has_value()) j["location"] = loc.location.value();
    // NOTE: API typo — the field is "additonal_info" (missing 'i'), Resolution H-09, G-07
    if (loc.additional_info.has_value()) j["additonal_info"] = loc.additional_info.value();
    return j;
}

struct OneOffEventTypeRequest {
    std::string name;
    std::string host;            // User URI
    std::vector<std::string> co_hosts;  // User URIs
    int duration = 30;           // Minutes
    std::string timezone;        // IANA tz
    DateSetting date_setting;
    OneOffLocation location;
};

inline json to_json_obj(const OneOffEventTypeRequest& req) {
    json j;
    j["name"] = req.name;
    j["host"] = req.host;
    j["co_hosts"] = req.co_hosts;
    j["duration"] = req.duration;
    j["timezone"] = req.timezone;
    j["date_setting"] = to_json_obj(req.date_setting);
    j["location"] = to_json_obj(req.location);
    return j;
}

// Response is undocumented (Resolution G-08).
// All fields optional until verified against live API.
struct OneOffEventTypeResponse {
    std::optional<std::string> uri;
    std::optional<std::string> name;
    std::optional<std::string> scheduling_url;
    std::optional<int> duration;
    std::optional<std::string> kind;
    std::optional<std::string> slug;
    std::optional<bool> secret;
    std::optional<std::string> created_at;
    std::optional<std::string> updated_at;
    json raw;                    // Store raw JSON alongside typed fields
};

inline void from_json(const json& j, OneOffEventTypeResponse& r) {
    if (j.contains("uri") && !j["uri"].is_null()) r.uri = j["uri"].get<std::string>();
    if (j.contains("name") && !j["name"].is_null()) r.name = j["name"].get<std::string>();
    if (j.contains("scheduling_url") && !j["scheduling_url"].is_null()) r.scheduling_url = j["scheduling_url"].get<std::string>();
    if (j.contains("duration") && !j["duration"].is_null()) r.duration = j["duration"].get<int>();
    if (j.contains("kind") && !j["kind"].is_null()) r.kind = j["kind"].get<std::string>();
    if (j.contains("slug") && !j["slug"].is_null()) r.slug = j["slug"].get<std::string>();
    if (j.contains("secret") && !j["secret"].is_null()) r.secret = j["secret"].get<bool>();
    if (j.contains("created_at") && !j["created_at"].is_null()) r.created_at = j["created_at"].get<std::string>();
    if (j.contains("updated_at") && !j["updated_at"].is_null()) r.updated_at = j["updated_at"].get<std::string>();
    r.raw = j;
}
