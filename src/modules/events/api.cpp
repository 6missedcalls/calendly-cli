#include "modules/events/api.h"
#include "core/rest_client.h"

namespace events_api {

CursorPage<ScheduledEvent> list(const ListOptions& opts) {
    std::map<std::string, std::string> params;
    params["user"] = opts.user_uri;
    params["count"] = std::to_string(opts.count);

    if (opts.status.has_value()) {
        params["status"] = opts.status.value();
    }
    if (opts.min_start_time.has_value()) {
        params["min_start_time"] = opts.min_start_time.value();
    }
    if (opts.max_start_time.has_value()) {
        params["max_start_time"] = opts.max_start_time.value();
    }
    if (opts.page_token.has_value()) {
        params["page_token"] = opts.page_token.value();
    }
    if (opts.sort.has_value()) {
        params["sort"] = opts.sort.value();
    }

    auto response = rest_get("scheduled_events", params);
    return response.get<CursorPage<ScheduledEvent>>();
}

ScheduledEvent get(const std::string& uuid) {
    auto response = rest_get("scheduled_events/" + uuid);
    // Response wrapped in {"resource": {...}}
    return response["resource"].get<ScheduledEvent>();
}

CreateEventResponse create(const CreateEventRequest& req) {
    auto body = to_json_obj(req);
    auto response = rest_post("scheduled_events", body);
    return response.get<CreateEventResponse>();
}

CancellationResponse cancel(const std::string& uuid, const CancellationRequest& req) {
    auto body = to_json_obj(req);
    auto response = rest_post("scheduled_events/" + uuid + "/cancellation", body);
    return response.get<CancellationResponse>();
}

}  // namespace events_api
