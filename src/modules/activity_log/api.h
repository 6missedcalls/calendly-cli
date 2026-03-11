#pragma once
#include <optional>
#include <string>
#include "modules/activity_log/model.h"

namespace activity_log_api {

struct ListOptions {
    std::optional<std::string> organization;  // Org URI (auto-resolved)
    std::optional<std::string> action;
    std::optional<std::string> actor_uri;
    std::optional<std::string> namespace_val;
    std::optional<std::string> min_occurred_at;
    std::optional<std::string> max_occurred_at;
    int count = 20;
    std::optional<std::string> page_token;
    std::optional<std::string> sort;
};

// GET /activity_log_entries
ActivityLogResponse list(const ListOptions& opts);

}  // namespace activity_log_api
