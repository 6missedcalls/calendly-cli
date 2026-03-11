#include "modules/invitees/api.h"
#include "core/rest_client.h"

namespace invitees_api {

CursorPage<Invitee> list(const ListOptions& opts) {
    std::map<std::string, std::string> params;
    params["count"] = std::to_string(opts.count);
    if (opts.page_token.has_value()) params["page_token"] = opts.page_token.value();
    if (opts.status.has_value()) params["status"] = opts.status.value();
    if (opts.sort.has_value()) params["sort"] = opts.sort.value();

    auto response = rest_get("scheduled_events/" + opts.event_uuid + "/invitees", params);
    return response.get<CursorPage<Invitee>>();
}

InviteRecord get(const std::string& invitee_uuid) {
    auto response = rest_get("invitees/" + invitee_uuid);
    // Response may be wrapped in {"resource": {...}} — handle both shapes
    if (response.contains("resource")) {
        return response["resource"].get<InviteRecord>();
    }
    return response.get<InviteRecord>();
}

BookingResponse book(const json& request_body) {
    auto response = rest_post("invitees", request_body);
    return response.get<BookingResponse>();
}

}  // namespace invitees_api
