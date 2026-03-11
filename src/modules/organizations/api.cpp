#include "modules/organizations/api.h"
#include "core/rest_client.h"
#include <map>

namespace organizations_api {

CursorPage<OrganizationInvitation> list_invitations(const ListInvitationsOptions& opts) {
    std::map<std::string, std::string> params;
    params["count"] = std::to_string(opts.count);
    if (opts.page_token.has_value()) params["page_token"] = opts.page_token.value();
    if (opts.status.has_value()) params["status"] = opts.status.value();

    auto response = rest_get("organizations/" + opts.org_uuid + "/invitations", params);
    return response.get<CursorPage<OrganizationInvitation>>();
}

}  // namespace organizations_api
