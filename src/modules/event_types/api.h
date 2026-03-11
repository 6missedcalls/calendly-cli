#pragma once
#include <map>
#include <optional>
#include <string>
#include <vector>
#include "core/types.h"
#include "modules/event_types/model.h"

namespace event_types_api {

struct ListOptions {
    std::string user_uri;           // Required: user URI filter
    std::optional<bool> active;     // Filter by active status
    int count = 20;
    std::optional<std::string> page_token;
    std::optional<std::string> sort;
};

// GET /event_types
CursorPage<EventType> list(const ListOptions& opts);

// GET /event_types/{uuid}
EventType get(const std::string& uuid);

struct AvailableTimesOptions {
    std::string event_type_uri;     // Required: full event type URI
    std::string start_time;         // ISO 8601
    std::string end_time;           // ISO 8601 (max 7 days from start)
};

// GET /event_type_available_times
// NOT paginated — returns flat collection
std::vector<AvailableTime> available_times(const AvailableTimesOptions& opts);

}  // namespace event_types_api
