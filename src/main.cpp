#include <CLI/CLI.hpp>
#include <curl/curl.h>
#include <iostream>
#include <stdexcept>

#include "core/cache.h"
#include "core/color.h"
#include "core/config.h"
#include "core/error.h"
#include "core/output.h"
#include "core/version.h"
#include "modules/users/commands.h"
#include "modules/event_types/commands.h"
#include "modules/events/commands.h"
#include "modules/invitees/commands.h"
#include "modules/organizations/commands.h"
#include "modules/activity_log/commands.h"
#include "modules/one_off/commands.h"
#include "modules/book/commands.h"
#include "modules/config/commands.h"
#include "modules/cache/commands.h"

static const char* CALENDLY_BANNER = R"(
   ██████╗ █████╗ ██╗     ███████╗███╗   ██╗██████╗ ██╗  ██╗   ██╗
  ██╔════╝██╔══██╗██║     ██╔════╝████╗  ██║██╔══██╗██║  ╚██╗ ██╔╝
  ██║     ███████║██║     █████╗  ██╔██╗ ██║██║  ██║██║   ╚████╔╝
  ██║     ██╔══██║██║     ██╔══╝  ██║╚██╗██║██║  ██║██║    ╚██╔╝
  ╚██████╗██║  ██║███████╗███████╗██║ ╚████║██████╔╝███████╗██║
   ╚═════╝╚═╝  ╚═╝╚══════╝╚══════╝╚═╝  ╚═══╝╚═════╝ ╚══════╝╚═╝
)";

int main(int argc, char** argv) {
    // Bare `calendly` -- show splash
    if (argc == 1) {
        std::cout << CALENDLY_BANNER
                  << "  Calendly CLI for humans and AI agents\n"
                  << "  built by 6missedcalls\n"
                  << "  v" << CALENDLY_VERSION << "\n\n"
                  << "  Run 'calendly --help' for available commands\n"
                  << "  Run 'calendly <command> --help' for command details\n"
                  << "\n";
        return 0;
    }

    curl_global_init(CURL_GLOBAL_DEFAULT);

    // Apply config defaults before CLI parsing (CLI flags override)
    CalendlyConfig user_config;
    if (config_exists()) {
        user_config = load_config();
        if (!user_config.color_enabled) {
            color::set_enabled(false);
        }
    }

    CLI::App app{"Calendly CLI for humans and AI agents"};
    app.add_flag_function("--version", [](int) {
        std::cout << CALENDLY_BANNER
                  << "  v" << CALENDLY_VERSION << "\n"
                  << "  built by 6missedcalls\n";
        throw CLI::Success();
    }, "Show version");
    app.require_subcommand();
    app.fallthrough();
    app.get_formatter()->column_width(32);

    // Global flags (default_count comes from config)
    int default_count = user_config.default_count;
    bool json_output = false, csv_output = false, no_color = false, no_cache = false, verbose = false;
    auto* json_flag = app.add_flag("--json", json_output, "Output raw JSON");
    auto* csv_flag = app.add_flag("--csv", csv_output, "Output CSV");
    json_flag->excludes(csv_flag);
    csv_flag->excludes(json_flag);
    app.add_flag("--no-color", no_color, "Disable colors");
    app.add_flag("--no-cache", no_cache, "Bypass cache");
    app.add_flag("--verbose", verbose, "Show debug info");

    app.parse_complete_callback([&]() {
        if (json_output) set_output_format(OutputFormat::Json);
        if (csv_output) set_output_format(OutputFormat::Csv);
        if (no_color) color::set_enabled(false);
        if (no_cache) set_cache_disabled(true);
        if (verbose) set_verbose(true);
        set_default_count(default_count);
    });

    // Register modules -- Scheduling
    book_commands::register_commands(app);
    events_commands::register_commands(app);
    event_types_commands::register_commands(app);
    one_off_commands::register_commands(app);

    // Register modules -- Data
    invitees_commands::register_commands(app);
    organizations_commands::register_commands(app);
    activity_log_commands::register_commands(app);

    // Register modules -- System
    users_commands::register_commands(app);
    config_commands::register_commands(app);
    cache_commands::register_commands(app);

    // Group subcommands for help display
    // Note: These must match the subcommand names registered by each module
    if (auto* sub = app.get_subcommand_no_throw("book")) sub->group("Scheduling");
    if (auto* sub = app.get_subcommand_no_throw("events")) sub->group("Scheduling");
    if (auto* sub = app.get_subcommand_no_throw("event-types")) sub->group("Scheduling");
    if (auto* sub = app.get_subcommand_no_throw("one-off")) sub->group("Scheduling");

    if (auto* sub = app.get_subcommand_no_throw("invitees")) sub->group("Data");
    if (auto* sub = app.get_subcommand_no_throw("org")) sub->group("Data");
    if (auto* sub = app.get_subcommand_no_throw("activity-log")) sub->group("Data");

    if (auto* sub = app.get_subcommand_no_throw("me")) sub->group("System");
    if (auto* sub = app.get_subcommand_no_throw("config")) sub->group("System");
    if (auto* sub = app.get_subcommand_no_throw("cache")) sub->group("System");

    // Parse and execute
    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        curl_global_cleanup();
        return app.exit(e);
    } catch (const CalendlyError&) {
        // Error already printed by command handler
        curl_global_cleanup();
        return 1;
    } catch (const std::invalid_argument& e) {
        print_error(std::string("Invalid input: ") + e.what());
        curl_global_cleanup();
        return 1;
    } catch (const std::exception& e) {
        print_error(std::string("Unexpected error: ") + e.what());
        curl_global_cleanup();
        return 1;
    }

    curl_global_cleanup();
    return 0;
}
