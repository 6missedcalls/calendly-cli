#pragma once
#include <optional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct EventTypeProfile {
    std::string type;    // "User" or "Team"
    std::string name;
    std::string owner;   // User or org URI
};

inline void from_json(const json& j, EventTypeProfile& p) {
    p.type = j.value("type", "");
    p.name = j.value("name", "");
    p.owner = j.value("owner", "");
}

struct Location {
    std::string kind;
    std::optional<std::string> location;
    std::optional<std::string> additional_info;
    std::optional<std::string> phone_number;
};

inline void from_json(const json& j, Location& l) {
    l.kind = j.value("kind", "");
    if (j.contains("location") && !j["location"].is_null()) l.location = j["location"].get<std::string>();
    if (j.contains("additional_info") && !j["additional_info"].is_null()) l.additional_info = j["additional_info"].get<std::string>();
    if (j.contains("phone_number") && !j["phone_number"].is_null()) l.phone_number = j["phone_number"].get<std::string>();
}

struct CustomQuestion {
    std::string name;
    std::string type;    // "text", "dropdown", "radio", "checkbox", "phone_number"
    int position = 0;
    bool enabled = true;
    bool required = false;
    std::vector<std::string> answer_choices;
    bool include_other = false;
};

inline void from_json(const json& j, CustomQuestion& q) {
    q.name = j.value("name", "");
    q.type = j.value("type", "text");
    q.position = j.value("position", 0);
    q.enabled = j.value("enabled", true);
    q.required = j.value("required", false);
    if (j.contains("answer_choices") && j["answer_choices"].is_array()) {
        q.answer_choices = j["answer_choices"].get<std::vector<std::string>>();
    }
    q.include_other = j.value("include_other", false);
}

struct EventType {
    std::string uri;
    std::string name;
    bool active = true;
    std::optional<std::string> booking_method;
    std::string slug;
    std::string scheduling_url;
    int duration = 0;
    std::string kind;          // "solo" or "group"
    std::optional<std::string> pooling_type;
    std::string type;          // Always "EventType"
    std::string color;
    std::string created_at;
    std::string updated_at;
    std::optional<std::string> internal_note;
    std::optional<std::string> description_plain;
    std::optional<std::string> description_html;
    EventTypeProfile profile;
    bool secret = false;
    std::optional<std::string> deleted_at;
    bool admin_managed = false;
    std::vector<CustomQuestion> custom_questions;
    std::optional<bool> is_paid;
    std::vector<int> duration_options;
    std::vector<Location> locations;
};

inline void from_json(const json& j, EventType& et) {
    et.uri = j.value("uri", "");
    et.name = j.value("name", "");
    et.active = j.value("active", true);
    if (j.contains("booking_method") && !j["booking_method"].is_null()) et.booking_method = j["booking_method"].get<std::string>();
    et.slug = j.value("slug", "");
    et.scheduling_url = j.value("scheduling_url", "");
    et.duration = j.value("duration", 0);
    et.kind = j.value("kind", "solo");
    if (j.contains("pooling_type") && !j["pooling_type"].is_null()) et.pooling_type = j["pooling_type"].get<std::string>();
    et.type = j.value("type", "EventType");
    et.color = j.value("color", "");
    et.created_at = j.value("created_at", "");
    et.updated_at = j.value("updated_at", "");
    if (j.contains("internal_note") && !j["internal_note"].is_null()) et.internal_note = j["internal_note"].get<std::string>();
    if (j.contains("description_plain") && !j["description_plain"].is_null()) et.description_plain = j["description_plain"].get<std::string>();
    if (j.contains("description_html") && !j["description_html"].is_null()) et.description_html = j["description_html"].get<std::string>();
    if (j.contains("profile")) et.profile = j["profile"].get<EventTypeProfile>();
    et.secret = j.value("secret", false);
    if (j.contains("deleted_at") && !j["deleted_at"].is_null()) et.deleted_at = j["deleted_at"].get<std::string>();
    et.admin_managed = j.value("admin_managed", false);
    if (j.contains("custom_questions") && j["custom_questions"].is_array()) {
        et.custom_questions = j["custom_questions"].get<std::vector<CustomQuestion>>();
    }
    if (j.contains("is_paid") && !j["is_paid"].is_null()) et.is_paid = j["is_paid"].get<bool>();
    if (j.contains("duration_options") && j["duration_options"].is_array()) {
        et.duration_options = j["duration_options"].get<std::vector<int>>();
    }
    if (j.contains("locations") && j["locations"].is_array()) {
        et.locations = j["locations"].get<std::vector<Location>>();
    }
}

struct AvailableTime {
    std::string status;
    std::string start_time;
    int invitees_remaining = 1;
};

inline void from_json(const json& j, AvailableTime& at) {
    at.status = j.value("status", "available");
    at.start_time = j.value("start_time", "");
    at.invitees_remaining = j.value("invitees_remaining", 1);
}
