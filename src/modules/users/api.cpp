#include "modules/users/api.h"
#include "core/rest_client.h"

namespace users_api {

User get_me() {
    auto response = rest_get("users/me");
    return response["resource"].get<User>();
}

}  // namespace users_api
