#include "modules/activity_log/api.h"
#include "core/rest_client.h"

namespace activity_log_api {

ActivityLogResponse list(const ListOptions& opts) {
    std::map<std::string, std::string> params;

    if (opts.organization.has_value()) {
        params["organization"] = opts.organization.value();
    }
    if (opts.action.has_value()) {
        params["action"] = opts.action.value();
    }
    if (opts.actor_uri.has_value()) {
        params["actor"] = opts.actor_uri.value();
    }
    if (opts.namespace_val.has_value()) {
        params["namespace"] = opts.namespace_val.value();
    }
    if (opts.min_occurred_at.has_value()) {
        params["min_occurred_at"] = opts.min_occurred_at.value();
    }
    if (opts.max_occurred_at.has_value()) {
        params["max_occurred_at"] = opts.max_occurred_at.value();
    }
    params["count"] = std::to_string(opts.count);
    if (opts.page_token.has_value()) {
        params["page_token"] = opts.page_token.value();
    }
    if (opts.sort.has_value()) {
        params["sort"] = opts.sort.value();
    }

    auto response = rest_get("activity_log_entries", params);
    return response.get<ActivityLogResponse>();
}

}  // namespace activity_log_api
