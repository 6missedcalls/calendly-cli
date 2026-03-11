#pragma once
#include "modules/one_off/model.h"

namespace one_off_api {

// POST /one_off_event_types
OneOffEventTypeResponse create(const OneOffEventTypeRequest& req);

}  // namespace one_off_api
