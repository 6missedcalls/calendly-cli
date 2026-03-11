#include "modules/one_off/api.h"
#include "core/rest_client.h"

namespace one_off_api {

OneOffEventTypeResponse create(const OneOffEventTypeRequest& req) {
    auto body = to_json_obj(req);
    auto response = rest_post("one_off_event_types", body);
    // Response may be wrapped in {"resource": {...}} — handle both
    if (response.contains("resource") && response["resource"].is_object()) {
        return response["resource"].get<OneOffEventTypeResponse>();
    }
    return response.get<OneOffEventTypeResponse>();
}

}  // namespace one_off_api
