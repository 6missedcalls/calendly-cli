#include "modules/book/commands.h"
#include "modules/events/model.h"
#include "modules/event_types/api.h"
#include "core/cache.h"
#include "core/color.h"
#include "core/config.h"
#include "core/error.h"
#include "core/filter.h"
#include "core/output.h"
#include "core/resolver.h"
#include "core/rest_client.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include "core/platform.h"

namespace book_commands {

namespace {

std::string now_iso8601() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm utc_tm{};
    gmtime_r(&time_t_now, &utc_tm);
    std::ostringstream oss;
    oss << std::put_time(&utc_tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

std::string add_days_iso8601(int days) {
    auto now = std::chrono::system_clock::now();
    auto future = now + std::chrono::hours(24 * days);
    auto time_t_val = std::chrono::system_clock::to_time_t(future);
    std::tm utc_tm{};
    gmtime_r(&time_t_val, &utc_tm);
    std::ostringstream oss;
    oss << std::put_time(&utc_tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

std::string select_start_time(const std::string& event_type_uri, bool first_flag) {
    event_types_api::AvailableTimesOptions opts;
    opts.event_type_uri = event_type_uri;
    opts.start_time = now_iso8601();
    opts.end_time = add_days_iso8601(7);

    auto times = event_types_api::available_times(opts);

    // Filter to only available slots
    std::vector<AvailableTime> available;
    for (const auto& t : times) {
        if (t.status == "available") {
            available.push_back(t);
        }
    }

    if (available.empty()) {
        throw CalendlyError(ErrorKind::Validation,
            "No available time slots found in the next 7 days.");
    }

    if (first_flag) {
        return available[0].start_time;
    }

    // Show top 5 slots and prompt for selection
    size_t display_count = std::min(available.size(), static_cast<size_t>(5));

    std::cout << "\nAvailable time slots:\n\n";
    for (size_t i = 0; i < display_count; ++i) {
        std::cout << "  " << color::bold("[" + std::to_string(i + 1) + "]")
                  << "  " << available[i].start_time << "\n";
    }
    std::cout << "\nSelect a slot (1-" << display_count << "): ";

    std::string input;
    std::getline(std::cin, input);

    int choice = 0;
    try {
        choice = std::stoi(input);
    } catch (...) {
        throw CalendlyError(ErrorKind::Validation, "Invalid selection.");
    }

    if (choice < 1 || static_cast<size_t>(choice) > display_count) {
        throw CalendlyError(ErrorKind::Validation,
            "Selection out of range. Expected 1-" + std::to_string(display_count) + ".");
    }

    return available[static_cast<size_t>(choice - 1)].start_time;
}

std::vector<QuestionAndAnswer> parse_qa_pairs(const std::vector<std::string>& raw) {
    std::vector<QuestionAndAnswer> result;
    for (const auto& pair : raw) {
        auto delim = pair.find("::");
        if (delim == std::string::npos || delim == 0) {
            throw CalendlyError(ErrorKind::Validation,
                "Invalid --qa format: '" + pair + "'. Expected 'question::answer'");
        }
        QuestionAndAnswer qa;
        qa.question = pair.substr(0, delim);
        qa.answer = pair.substr(delim + 2);
        qa.position = static_cast<int>(result.size());
        result.push_back(qa);
    }
    return result;
}

void handle_book(const std::string& type_identifier,
                 const std::string& name,
                 const std::string& email,
                 const std::string& timezone_opt,
                 const std::string& first_name_opt,
                 const std::string& last_name_opt,
                 const std::string& start_time_opt,
                 bool first_flag,
                 const std::string& location_kind_opt,
                 const std::string& location_opt,
                 const std::vector<std::string>& guests,
                 const std::vector<std::string>& qa_pairs,
                 bool dry_run,
                 bool yes) {
    auto format = get_output_format();

    try {
        Cache cache;
        Resolver resolver(cache);

        // Ensure user is cached, then read profile for defaults
        resolver.get_user_uri();
        auto cached_user = cache.get_user();
        std::string user_timezone = cached_user.has_value()
            ? cached_user->timezone : "";

        // Resolve timezone: flag > config > user profile
        std::string config_timezone;
        if (config_exists()) {
            auto cfg = load_config();
            if (cfg.default_timezone.has_value()) {
                config_timezone = cfg.default_timezone.value();
            }
        }
        std::string timezone = !timezone_opt.empty() ? timezone_opt
            : !config_timezone.empty() ? config_timezone
            : user_timezone;

        // Resolve event type: name, slug, UUID prefix, UUID, or full URI
        std::string event_type_uri = resolver.resolve_event_type(type_identifier);

        // Resolve start time
        // When --yes is passed or stdin is not a TTY, auto-pick first slot
        bool auto_first = first_flag || yes || !isatty(STDIN_FILENO);
        std::string start_time;
        if (!start_time_opt.empty()) {
            start_time = start_time_opt;
        } else {
            start_time = select_start_time(event_type_uri, auto_first);
        }

        // Build invitee request
        InviteeRequest invitee;
        invitee.name = name;
        invitee.email = email;
        invitee.timezone = timezone;
        if (!first_name_opt.empty()) { invitee.first_name = first_name_opt; }
        if (!last_name_opt.empty()) { invitee.last_name = last_name_opt; }

        // Build create event request
        CreateEventRequest req;
        req.event_type = event_type_uri;
        req.start_time = start_time;
        req.invitee = invitee;
        req.event_guests = guests;
        req.questions_and_answers = parse_qa_pairs(qa_pairs);

        if (!location_kind_opt.empty()) {
            LocationRequest loc;
            loc.kind = location_kind_opt;
            if (!location_opt.empty()) { loc.location = location_opt; }
            req.location = loc;
        }

        auto request_body = to_json_obj(req);

        // Dry run: output the request and exit
        if (dry_run) {
            std::cout << color::dim("-- Dry run: request body --") << "\n";
            output_json(request_body, std::cout);
            return;
        }

        // Send the booking request
        auto response = rest_post("scheduled_events", request_body);

        if (format == OutputFormat::Json) {
            output_json(response, std::cout);
            return;
        }

        // Parse response
        auto created = response.get<CreateEventResponse>();

        // Display result
        DetailRenderer detail;
        detail.add_section("Booking Confirmed");
        detail.add_field("Event URI", created.resource);
        detail.add_field("Invitee Name", name);
        detail.add_field("Invitee Email", email);
        detail.add_field("Start Time", start_time);

        if (created.invitee.is_object()) {
            if (created.invitee.contains("cancel_url") && !created.invitee["cancel_url"].is_null()) {
                detail.add_field("Cancel URL", created.invitee["cancel_url"].get<std::string>());
            }
            if (created.invitee.contains("reschedule_url") && !created.invitee["reschedule_url"].is_null()) {
                detail.add_field("Reschedule URL", created.invitee["reschedule_url"].get<std::string>());
            }
        }

        detail.render(std::cout);

    } catch (const CalendlyError& e) {
        print_error(format_error(e));
        throw;
    }
}

}  // namespace

void register_commands(CLI::App& app) {
    auto* book_cmd = app.add_subcommand("book", "Book a Calendly event");

    auto* type_val = new std::string();
    book_cmd->add_option("--type", *type_val, "Event type name, slug, UUID, or URI")
        ->required();

    auto* name_val = new std::string();
    book_cmd->add_option("--name", *name_val, "Invitee name")
        ->required();

    auto* email_val = new std::string();
    book_cmd->add_option("--email", *email_val, "Invitee email")
        ->required();

    auto* timezone_val = new std::string();
    book_cmd->add_option("--timezone", *timezone_val, "Invitee timezone (default: from user profile)");

    auto* first_name_val = new std::string();
    book_cmd->add_option("--first-name", *first_name_val, "Invitee first name");

    auto* last_name_val = new std::string();
    book_cmd->add_option("--last-name", *last_name_val, "Invitee last name");

    auto* start_time_val = new std::string();
    book_cmd->add_option("--start-time", *start_time_val, "Start time (ISO 8601)");

    auto* first_flag_val = new bool(false);
    book_cmd->add_flag("--first", *first_flag_val, "Auto-pick first available slot");

    auto* location_kind_val = new std::string();
    book_cmd->add_option("--location-kind", *location_kind_val, "Location kind");

    auto* location_val = new std::string();
    book_cmd->add_option("--location", *location_val, "Location value");

    auto* guests_val = new std::vector<std::string>();
    book_cmd->add_option("--guest", *guests_val, "Additional guest email (repeatable)");

    auto* qa_val = new std::vector<std::string>();
    book_cmd->add_option("--qa", *qa_val, "Question::Answer pair (repeatable, use :: delimiter)");

    auto* dry_run_val = new bool(false);
    book_cmd->add_flag("--dry-run", *dry_run_val, "Show request without sending");

    auto* yes_val = new bool(false);
    book_cmd->add_flag("--yes,-y", *yes_val, "Skip confirmation, auto-pick first slot");

    book_cmd->callback([type_val, name_val, email_val, timezone_val,
                        first_name_val, last_name_val, start_time_val,
                        first_flag_val, location_kind_val, location_val,
                        guests_val, qa_val, dry_run_val, yes_val]() {
        handle_book(
            *type_val,
            *name_val,
            *email_val,
            *timezone_val,
            *first_name_val,
            *last_name_val,
            *start_time_val,
            *first_flag_val,
            *location_kind_val,
            *location_val,
            *guests_val,
            *qa_val,
            *dry_run_val,
            *yes_val
        );
    });
}

}  // namespace book_commands
