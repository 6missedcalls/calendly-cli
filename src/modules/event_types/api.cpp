#include "modules/event_types/api.h"
#include "core/rest_client.h"

namespace event_types_api {

CursorPage<EventType> list(const ListOptions& opts) {
    std::map<std::string, std::string> params;
    params["user"] = opts.user_uri;
    params["count"] = std::to_string(opts.count);
    if (opts.active.has_value()) {
        params["active"] = opts.active.value() ? "true" : "false";
    }
    if (opts.page_token.has_value()) {
        params["page_token"] = opts.page_token.value();
    }
    if (opts.sort.has_value()) {
        params["sort"] = opts.sort.value();
    }

    auto response = rest_get("event_types", params);
    return response.get<CursorPage<EventType>>();
}

EventType get(const std::string& uuid) {
    auto response = rest_get("event_types/" + uuid);
    // Response wrapped in {"resource": {...}}
    return response["resource"].get<EventType>();
}

std::vector<AvailableTime> available_times(const AvailableTimesOptions& opts) {
    std::map<std::string, std::string> params;
    params["event_type"] = opts.event_type_uri;
    params["start_time"] = opts.start_time;
    params["end_time"] = opts.end_time;

    auto response = rest_get("event_type_available_times", params);
    // Response: {"collection": [...]} (NOT paginated)
    return response["collection"].get<std::vector<AvailableTime>>();
}

}  // namespace event_types_api
