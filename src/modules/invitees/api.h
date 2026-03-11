#pragma once
#include <optional>
#include <string>
#include "core/types.h"
#include "modules/invitees/model.h"

namespace invitees_api {

struct ListOptions {
    std::string event_uuid;      // Required: scheduled event UUID
    int count = 20;
    std::optional<std::string> page_token;
    std::optional<std::string> status;   // Filter: "active" or "canceled"
    std::optional<std::string> sort;
};

// GET /scheduled_events/{uuid}/invitees
CursorPage<Invitee> list(const ListOptions& opts);

// GET /invitees/{invitee_uuid}
// Returns distinct InviteRecord type (Resolution B-04)
InviteRecord get(const std::string& invitee_uuid);

// POST /invitees (booking)
// Uses CreateEventRequest from events/model.h as request body
BookingResponse book(const json& request_body);

}  // namespace invitees_api
