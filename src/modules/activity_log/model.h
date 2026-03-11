#pragma once
#include <optional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "core/types.h"

using json = nlohmann::json;

struct ActorOrganization {
    std::string uri;
    std::string role;            // "Owner", "Admin", "User"
};

inline void from_json(const json& j, ActorOrganization& ao) {
    ao.uri = j.value("uri", "");
    ao.role = j.value("role", "");
}

struct ActorGroup {
    std::string uri;
    std::string name;
    std::string role;            // "Admin", "Member"
};

inline void from_json(const json& j, ActorGroup& ag) {
    ag.uri = j.value("uri", "");
    ag.name = j.value("name", "");
    ag.role = j.value("role", "");
}

struct ActivityLogActor {
    std::string uri;
    std::string type;            // Typically "User"
    std::optional<ActorOrganization> organization;  // Nullable (Resolution H-07)
    std::optional<ActorGroup> group;                // Nullable (Resolution H-07)
    std::string display_name;
    std::string alternative_identifier;  // Typically email
};

inline void from_json(const json& j, ActivityLogActor& a) {
    a.uri = j.value("uri", "");
    a.type = j.value("type", "");
    if (j.contains("organization") && !j["organization"].is_null()) {
        a.organization = j["organization"].get<ActorOrganization>();
    }
    if (j.contains("group") && !j["group"].is_null()) {
        a.group = j["group"].get<ActorGroup>();
    }
    a.display_name = j.value("display_name", "");
    a.alternative_identifier = j.value("alternative_identifier", "");
}

struct ActivityLogEntry {
    std::string occurred_at;
    std::string uri;
    std::string namespace_val;   // "User", "EventType", etc.
    std::string action;          // "Add", "Remove", etc.
    ActivityLogActor actor;
    std::string fully_qualified_name;  // "Namespace.Action"
    json details;                // Opaque, dynamic structure
    std::string organization;    // Org URI
};

inline void from_json(const json& j, ActivityLogEntry& e) {
    e.occurred_at = j.value("occurred_at", "");
    e.uri = j.value("uri", "");
    e.namespace_val = j.value("namespace", "");
    e.action = j.value("action", "");
    if (j.contains("actor")) e.actor = j["actor"].get<ActivityLogActor>();
    e.fully_qualified_name = j.value("fully_qualified_name", "");
    if (j.contains("details")) e.details = j["details"];
    e.organization = j.value("organization", "");
}

// Extended response (Resolution H-02) -- NOT a generic CursorPage
struct ActivityLogResponse {
    std::vector<ActivityLogEntry> collection;
    CursorPageInfo pagination;
    std::optional<std::string> last_event_time;
    int total_count = 0;
    bool exceeds_max_total_count = false;
};

inline void from_json(const json& j, ActivityLogResponse& r) {
    if (j.contains("collection") && j["collection"].is_array()) {
        r.collection = j["collection"].get<std::vector<ActivityLogEntry>>();
    }
    if (j.contains("pagination")) {
        r.pagination = j["pagination"].get<CursorPageInfo>();
    }
    if (j.contains("last_event_time") && !j["last_event_time"].is_null()) {
        r.last_event_time = j["last_event_time"].get<std::string>();
    }
    r.total_count = j.value("total_count", 0);
    r.exceeds_max_total_count = j.value("exceeds_max_total_count", false);
}
