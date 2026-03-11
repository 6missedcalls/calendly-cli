#include "modules/one_off/commands.h"
#include "modules/one_off/api.h"
#include "core/error.h"
#include "core/filter.h"
#include "core/output.h"
#include "core/rest_client.h"

namespace one_off_commands {

namespace {

struct UserProfile {
    std::string uri;
    std::string timezone;
};

UserProfile get_current_user_profile() {
    auto response = rest_get("users/me");
    const auto& resource = response["resource"];
    return {
        resource["uri"].get<std::string>(),
        resource["timezone"].get<std::string>()
    };
}

void handle_create(const std::string& name,
                   int duration,
                   const std::string& start_date,
                   const std::string& end_date,
                   const std::string& location_kind,
                   const std::vector<std::string>& co_hosts,
                   const std::string& timezone_opt,
                   const std::string& location_opt,
                   const std::string& additional_info_opt) {
    auto format = get_output_format();

    try {
        auto profile = get_current_user_profile();

        std::string timezone = timezone_opt.empty()
            ? profile.timezone
            : timezone_opt;

        OneOffEventTypeRequest req;
        req.name = name;
        req.host = profile.uri;
        req.co_hosts = co_hosts;
        req.duration = duration;
        req.timezone = timezone;
        req.date_setting = DateSetting{"date_range", start_date, end_date};

        OneOffLocation loc;
        loc.kind = location_kind;
        if (!location_opt.empty()) loc.location = location_opt;
        if (!additional_info_opt.empty()) loc.additional_info = additional_info_opt;
        req.location = loc;

        auto result = one_off_api::create(req);

        if (format == OutputFormat::Json) {
            output_json(result.raw, std::cout);
            return;
        }

        // Detail view
        DetailRenderer detail;
        detail.add_field("URI", result.uri);
        detail.add_field("Name", result.name);
        detail.add_field("Scheduling URL", result.scheduling_url);
        if (result.duration.has_value()) {
            detail.add_field("Duration", std::to_string(result.duration.value()) + " minutes");
        }
        detail.add_field("Kind", result.kind);
        detail.add_field("Slug", result.slug);
        detail.render(std::cout);

    } catch (const CalendlyError& e) {
        print_error(format_error(e));
        throw;
    }
}

}  // namespace

void register_commands(CLI::App& app) {
    auto* oo_group = app.add_subcommand("one-off", "Manage one-off event types");
    oo_group->require_subcommand(1);

    // --- create ---
    auto* create_cmd = oo_group->add_subcommand("create", "Create a one-off event type");

    auto* name_val = new std::string();
    create_cmd->add_option("--name", *name_val, "Event type name")
        ->required();

    auto* duration_val = new int(30);
    create_cmd->add_option("--duration", *duration_val, "Duration in minutes")
        ->default_val(30);

    auto* start_date_val = new std::string();
    create_cmd->add_option("--start-date", *start_date_val, "Start date (YYYY-MM-DD)")
        ->required();

    auto* end_date_val = new std::string();
    create_cmd->add_option("--end-date", *end_date_val, "End date (YYYY-MM-DD)")
        ->required();

    auto* location_kind_val = new std::string();
    create_cmd->add_option("--location-kind", *location_kind_val, "Location kind (e.g. physical, custom)")
        ->required();

    auto* co_hosts_val = new std::vector<std::string>();
    create_cmd->add_option("--co-host", *co_hosts_val, "Co-host user URI (repeatable)");

    auto* timezone_val = new std::string();
    create_cmd->add_option("--timezone", *timezone_val, "IANA timezone (default: from user profile)");

    auto* location_val = new std::string();
    create_cmd->add_option("--location", *location_val, "Location details");

    auto* additional_info_val = new std::string();
    create_cmd->add_option("--additional-info", *additional_info_val, "Additional location info");

    create_cmd->callback([name_val, duration_val, start_date_val, end_date_val,
                          location_kind_val, co_hosts_val, timezone_val,
                          location_val, additional_info_val]() {
        handle_create(
            *name_val,
            *duration_val,
            *start_date_val,
            *end_date_val,
            *location_kind_val,
            *co_hosts_val,
            *timezone_val,
            *location_val,
            *additional_info_val
        );
    });
}

}  // namespace one_off_commands
