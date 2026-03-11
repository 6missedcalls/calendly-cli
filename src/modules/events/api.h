#pragma once
#include <map>
#include <optional>
#include <string>
#include "core/types.h"
#include "modules/events/model.h"

namespace events_api {

struct ListOptions {
    std::string user_uri;
    std::optional<std::string> status;     // "confirmed" or "cancelled"
    std::optional<std::string> min_start_time;
    std::optional<std::string> max_start_time;
    int count = 20;
    std::optional<std::string> page_token;
    std::optional<std::string> sort;
};

// GET /scheduled_events
CursorPage<ScheduledEvent> list(const ListOptions& opts);

// GET /scheduled_events/{uuid}
ScheduledEvent get(const std::string& uuid);

// POST /scheduled_events
CreateEventResponse create(const CreateEventRequest& req);

// POST /scheduled_events/{uuid}/cancellation
CancellationResponse cancel(const std::string& uuid, const CancellationRequest& req);

}  // namespace events_api
