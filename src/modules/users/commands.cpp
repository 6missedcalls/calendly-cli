#include "modules/users/commands.h"
#include "modules/users/api.h"
#include "core/color.h"
#include "core/error.h"
#include "core/filter.h"
#include "core/output.h"
#include "core/rest_client.h"

namespace users_commands {

static void handle_me() {
    auto format = get_output_format();

    try {
        if (format == OutputFormat::Json) {
            auto response = rest_get("users/me");
            output_json(response, std::cout);
            return;
        }

        auto user = users_api::get_me();

        if (format == OutputFormat::Csv) {
            output_csv_header({"name", "email", "timezone", "scheduling_url"});
            output_csv_row({user.name, user.email, user.timezone, user.scheduling_url});
            return;
        }

        // Detail view (default for single resource)
        DetailRenderer detail;
        detail.add_field("Name", user.name);
        detail.add_field("Email", user.email);
        detail.add_field("Slug", user.slug);
        detail.add_field("Timezone", user.timezone);
        detail.add_field("Time Notation", user.time_notation);
        detail.add_field("Scheduling URL", user.scheduling_url);
        detail.add_field("Avatar URL", user.avatar_url);
        detail.add_field("Locale", user.locale);
        detail.add_field("Organization", strip_base_url(user.current_organization));
        detail.add_field("Created", format_relative_time(user.created_at));
        detail.add_field("Updated", format_relative_time(user.updated_at));
        detail.add_field("URI", user.uri);
        detail.render(std::cout);

    } catch (const CalendlyError& e) {
        print_error(format_error(e));
        throw;
    }
}

void register_commands(CLI::App& app) {
    auto* me = app.add_subcommand("me", "Show current authenticated user");
    me->callback(handle_me);
}

}  // namespace users_commands
