#pragma once
#include <optional>
#include <string>
#include "core/types.h"
#include "modules/organizations/model.h"

namespace organizations_api {

struct ListInvitationsOptions {
    std::string org_uuid;        // Organization UUID (auto-resolved from user)
    int count = 20;
    std::optional<std::string> page_token;
    std::optional<std::string> status;  // "pending", "accepted", "declined"
};

// GET /organizations/{uuid}/invitations
CursorPage<OrganizationInvitation> list_invitations(const ListInvitationsOptions& opts);

}  // namespace organizations_api
