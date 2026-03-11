#include "modules/organizations/commands.h"
#include "modules/organizations/api.h"
#include "core/color.h"
#include "core/error.h"
#include "core/filter.h"
#include "core/output.h"
#include "core/paginator.h"
#include "core/rest_client.h"

namespace organizations_commands {

namespace {

std::string resolve_org_uuid() {
    auto response = rest_get("users/me");
    auto org_uri = response["resource"]["current_organization"].get<std::string>();
    return extract_uuid(org_uri);
}

void handle_invitations(int count, bool all, const std::string& status) {
    auto format = get_output_format();

    try {
        auto org_uuid = resolve_org_uuid();

        if (format == OutputFormat::Json) {
            std::map<std::string, std::string> params;
            params["count"] = std::to_string(validate_count(count));
            if (!status.empty()) {
                params["status"] = status;
            }
            auto response = rest_get("organizations/" + org_uuid + "/invitations", params);
            output_json(response, std::cout);
            return;
        }

        organizations_api::ListInvitationsOptions opts;
        opts.org_uuid = org_uuid;
        opts.count = validate_count(count);
        if (!status.empty()) {
            opts.status = status;
        }

        PaginationOptions page_opts;
        page_opts.count = opts.count;
        page_opts.fetch_all = all;

        CursorPaginator<OrganizationInvitation> paginator(
            [&opts](const std::optional<std::string>& token) {
                auto page_options = opts;
                page_options.page_token = token;
                return organizations_api::list_invitations(page_options);
            },
            page_opts
        );

        auto invitations = all ? paginator.fetch_all()
                               : paginator.fetch_page().collection;

        if (format == OutputFormat::Csv) {
            output_csv_header({"email", "status", "created_at", "last_sent_at"});
            for (const auto& inv : invitations) {
                output_csv_row({
                    inv.email,
                    inv.status,
                    inv.created_at,
                    inv.last_sent_at.value_or("")
                });
            }
            return;
        }

        // Table view
        TableRenderer table({
            {"EMAIL",        8, 35},
            {"STATUS",       6, 12},
            {"CREATED",      8, 25},
            {"LAST SENT",    8, 25}
        });

        for (const auto& inv : invitations) {
            table.add_row({
                inv.email,
                color::org_invitation_status(inv.status, inv.status),
                format_relative_time(inv.created_at),
                inv.last_sent_at.has_value()
                    ? format_relative_time(inv.last_sent_at.value())
                    : color::dim("never")
            });
        }

        if (table.empty()) {
            print_warning("No organization invitations found.");
            return;
        }

        table.render(std::cout);

    } catch (const CalendlyError& e) {
        print_error(format_error(e));
        throw;
    }
}

}  // namespace

void register_commands(CLI::App& app) {
    auto* org_group = app.add_subcommand("org", "Manage organization");
    org_group->require_subcommand(1);

    // --- invitations ---
    auto* inv_cmd = org_group->add_subcommand("invitations", "List organization invitations");

    auto* count_opt = new int(get_default_count());
    inv_cmd->add_option("--count", *count_opt, "Number of results per page (1-100)")
        ->default_val(20);

    auto* all_flag_val = new bool(false);
    inv_cmd->add_flag("--all", *all_flag_val, "Fetch all pages");

    auto* status_opt = new std::string();
    inv_cmd->add_option("--status", *status_opt, "Filter by status (pending, accepted, declined)");

    inv_cmd->callback([count_opt, all_flag_val, status_opt]() {
        handle_invitations(*count_opt, *all_flag_val, *status_opt);
    });
}

}  // namespace organizations_commands
