#pragma once
#include "modules/users/model.h"

namespace users_api {

// GET /users/me
// Response: {"resource": {...user object...}}
User get_me();

}  // namespace users_api
