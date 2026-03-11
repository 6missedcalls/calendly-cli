#include "modules/events/commands.h"
#include "modules/events/api.h"
#include "core/color.h"
#include "core/error.h"
#include "core/filter.h"
#include "core/output.h"
#include "core/paginator.h"
#include "core/rest_client.h"
#include "core/types.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include "core/platform.h"

namespace events_commands {

namespace {

// --- Time helpers ---

std::string utc_start_of_day(int day_offset) {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);

    std::tm utc_tm = {};
    gmtime_r(&time_t_now, &utc_tm);

    utc_tm.tm_hour = 0;
    utc_tm.tm_min = 0;
    utc_tm.tm_sec = 0;
    utc_tm.tm_mday += day_offset;

    auto adjusted = timegm(&utc_tm);
    std::tm result = {};
    gmtime_r(&adjusted, &result);

    std::ostringstream oss;
    oss << std::put_time(&result, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

std::string utc_end_of_day(int day_offset) {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);

    std::tm utc_tm = {};
    gmtime_r(&time_t_now, &utc_tm);

    utc_tm.tm_hour = 23;
    utc_tm.tm_min = 59;
    utc_tm.tm_sec = 59;
    utc_tm.tm_mday += day_offset;

    auto adjusted = timegm(&utc_tm);
    std::tm result = {};
    gmtime_r(&adjusted, &result);

    std::ostringstream oss;
    oss << std::put_time(&result, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

std::string utc_end_of_week() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);

    std::tm utc_tm = {};
    gmtime_r(&time_t_now, &utc_tm);

    // Days until Sunday (0=Sun, 1=Mon, ..., 6=Sat)
    int days_until_sunday = (7 - utc_tm.tm_wday) % 7;
    if (days_until_sunday == 0) {
        days_until_sunday = 7;
    }

    utc_tm.tm_mday += days_until_sunday;
    utc_tm.tm_hour = 23;
    utc_tm.tm_min = 59;
    utc_tm.tm_sec = 59;

    auto adjusted = timegm(&utc_tm);
    std::tm result = {};
    gmtime_r(&adjusted, &result);

    std::ostringstream oss;
    oss << std::put_time(&result, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

std::string utc_now_iso() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);

    std::tm utc_tm = {};
    gmtime_r(&time_t_now, &utc_tm);

    std::ostringstream oss;
    oss << std::put_time(&utc_tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

// --- Fetch current user URI ---

std::string fetch_user_uri() {
    auto response = rest_get("users/me");
    return response["resource"]["uri"].get<std::string>();
}

// --- Format helpers ---

std::string format_event_time(const std::string& iso8601) {
    return format_relative_time(iso8601);
}

std::string colored_status(const std::string& status) {
    return color::event_status(status, status);
}

// --- List handler ---

void handle_list(
    const std::string& status_filter,
    const std::string& min_start,
    const std::string& max_start,
    int count,
    bool fetch_all,
    const std::string& sort,
    bool today,
    bool tomorrow,
    bool this_week
) {
    auto format = get_output_format();

    try {
        auto user_uri = fetch_user_uri();

        // Resolve time filters from convenience flags
        std::optional<std::string> resolved_min = min_start.empty()
            ? std::nullopt
            : std::optional<std::string>(min_start);
        std::optional<std::string> resolved_max = max_start.empty()
            ? std::nullopt
            : std::optional<std::string>(max_start);

        if (today) {
            resolved_min = utc_start_of_day(0);
            resolved_max = utc_end_of_day(0);
        } else if (tomorrow) {
            resolved_min = utc_start_of_day(1);
            resolved_max = utc_end_of_day(1);
        } else if (this_week) {
            resolved_min = utc_now_iso();
            resolved_max = utc_end_of_week();
        }

        auto validated_count = validate_count(count);

        // Normalize British "cancelled" → API's "canceled"
        std::string normalized_status = (status_filter == "cancelled")
            ? "canceled" : status_filter;

        events_api::ListOptions list_opts;
        list_opts.user_uri = user_uri;
        list_opts.count = validated_count;
        list_opts.min_start_time = resolved_min;
        list_opts.max_start_time = resolved_max;

        if (!normalized_status.empty()) {
            list_opts.status = normalized_status;
        }
        if (!sort.empty()) {
            list_opts.sort = validate_sort(sort);
        }

        PaginationOptions page_opts;
        page_opts.count = validated_count;
        page_opts.fetch_all = fetch_all;

        CursorPaginator<ScheduledEvent> paginator(
            [&](const std::optional<std::string>& page_token) {
                auto opts = list_opts;
                opts.page_token = page_token;
                return events_api::list(opts);
            },
            page_opts
        );

        // JSON mode: output raw API response (single page)
        if (format == OutputFormat::Json && !fetch_all) {
            std::map<std::string, std::string> params;
            params["user"] = user_uri;
            params["count"] = std::to_string(validated_count);
            if (!normalized_status.empty()) params["status"] = normalized_status;
            if (resolved_min.has_value()) params["min_start_time"] = *resolved_min;
            if (resolved_max.has_value()) params["max_start_time"] = *resolved_max;
            if (!sort.empty()) params["sort"] = validate_sort(sort);
            auto response = rest_get("scheduled_events", params);
            output_json(response, std::cout);
            return;
        }

        if (fetch_all) {
            auto events = paginator.fetch_all();

            if (format == OutputFormat::Json) {
                json arr = json::array();
                for (const auto& ev : events) {
                    json j;
                    j["uri"] = ev.uri;
                    j["uuid"] = ev.uuid.value_or("");
                    j["start_time"] = ev.start_time;
                    j["end_time"] = ev.end_time;
                    j["status"] = ev.status;
                    j["event_type"] = ev.event_type;
                    j["location"] = ev.location.value_or("");
                    j["created_at"] = ev.created_at;
                    arr.push_back(j);
                }
                json result;
                result["collection"] = arr;
                result["total"] = events.size();
                output_json(result, std::cout);
                return;
            }

            if (format == OutputFormat::Csv) {
                output_csv_header({"uuid", "start_time", "end_time", "status", "location"});
                for (const auto& ev : events) {
                    output_csv_row({
                        ev.uuid.value_or(""),
                        ev.start_time,
                        ev.end_time,
                        ev.status,
                        ev.location.value_or("")
                    });
                }
                return;
            }

            TableRenderer table({
                {"UUID", 8, 12},
                {"Start", 10, 20},
                {"End", 10, 20},
                {"Status", 6, 12},
                {"Location", 8, 30}
            });

            for (const auto& ev : events) {
                table.add_row({
                    ev.uuid ? short_uuid(*ev.uuid) : "",
                    format_event_time(ev.start_time),
                    format_event_time(ev.end_time),
                    colored_status(ev.status),
                    ev.location.value_or("-")
                });
            }

            if (table.empty()) {
                print_warning("No events found.");
            } else {
                table.render(std::cout);
            }
            return;
        }

        // Single page
        auto page = paginator.fetch_page();

        if (format == OutputFormat::Csv) {
            output_csv_header({"uuid", "start_time", "end_time", "status", "location"});
            for (const auto& ev : page.collection) {
                output_csv_row({
                    ev.uuid.value_or(""),
                    ev.start_time,
                    ev.end_time,
                    ev.status,
                    ev.location.value_or("")
                });
            }
            return;
        }

        TableRenderer table({
            {"UUID", 8, 12},
            {"Start", 10, 20},
            {"End", 10, 20},
            {"Status", 6, 12},
            {"Location", 8, 30}
        });

        for (const auto& ev : page.collection) {
            table.add_row({
                ev.uuid ? short_uuid(*ev.uuid) : "",
                format_event_time(ev.start_time),
                format_event_time(ev.end_time),
                colored_status(ev.status),
                ev.location.value_or("-")
            });
        }

        if (table.empty()) {
            print_warning("No events found.");
        } else {
            table.render(std::cout);
        }

    } catch (const CalendlyError& e) {
        print_error(format_error(e));
        throw;
    }
}

// --- Show handler ---

void handle_show(const std::string& uuid) {
    auto format = get_output_format();

    try {
        if (format == OutputFormat::Json) {
            auto response = rest_get("scheduled_events/" + uuid);
            output_json(response, std::cout);
            return;
        }

        auto event = events_api::get(uuid);

        if (format == OutputFormat::Csv) {
            output_csv_header({"uuid", "start_time", "end_time", "status", "event_type", "location", "created_at"});
            output_csv_row({
                event.uuid.value_or(uuid),
                event.start_time,
                event.end_time,
                event.status,
                event.event_type,
                event.location.value_or(""),
                event.created_at
            });
            return;
        }

        DetailRenderer detail;
        detail.add_field("UUID", event.uuid.value_or(uuid));
        detail.add_field("URI", event.uri);
        detail.add_field("Status", colored_status(event.status));
        detail.add_field("Event Type", strip_base_url(event.event_type));

        detail.add_blank_line();
        detail.add_section("Schedule");
        detail.add_field("Start", format_event_time(event.start_time));
        detail.add_field("End", format_event_time(event.end_time));
        detail.add_field("Created", format_relative_time(event.created_at));

        if (event.location) {
            detail.add_blank_line();
            detail.add_section("Location");
            detail.add_field("Location", *event.location);
        }

        detail.add_blank_line();
        detail.add_section("Invitees");
        detail.add_field("Total", std::to_string(event.invitees_counter.total));
        detail.add_field("Confirmed", std::to_string(event.invitees_counter.confirmed));
        detail.add_field("Declined", std::to_string(event.invitees_counter.declined));

        if (!event.questions_and_answers.empty()) {
            detail.add_blank_line();
            detail.add_section("Questions & Answers");
            for (const auto& qa : event.questions_and_answers) {
                detail.add_field(qa.question, qa.answer);
            }
        }

        detail.render(std::cout);

    } catch (const CalendlyError& e) {
        print_error(format_error(e));
        throw;
    }
}

// --- Create handler ---

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

void handle_create(
    const std::string& event_type,
    const std::string& start_time,
    const std::string& name,
    const std::string& email,
    const std::string& timezone,
    const std::string& location_kind,
    const std::string& location,
    const std::vector<std::string>& guests,
    const std::vector<std::string>& qa_pairs,
    bool dry_run
) {
    try {
        CreateEventRequest req;
        req.event_type = event_type;
        req.start_time = start_time;
        req.invitee = InviteeRequest{
            .name = name,
            .first_name = std::nullopt,
            .last_name = std::nullopt,
            .email = email,
            .timezone = timezone,
            .text_reminder_number = std::nullopt
        };

        if (!location_kind.empty()) {
            LocationRequest loc_req;
            loc_req.kind = location_kind;
            if (!location.empty()) {
                loc_req.location = location;
            }
            req.location = loc_req;
        }

        req.event_guests = guests;
        req.questions_and_answers = parse_qa_pairs(qa_pairs);

        if (dry_run) {
            auto body = to_json_obj(req);
            output_json(body, std::cout);
            return;
        }

        auto format = get_output_format();
        auto response = events_api::create(req);

        if (format == OutputFormat::Json) {
            json j;
            j["resource"] = response.resource;
            j["invitee"] = response.invitee;
            output_json(j, std::cout);
            return;
        }

        DetailRenderer detail;
        detail.add_field("Event URI", response.resource);
        if (!response.invitee.is_null() && response.invitee.contains("uri")) {
            detail.add_field("Invitee URI", response.invitee["uri"].get<std::string>());
        }
        detail.render(std::cout);

        print_success("Event created successfully.");

    } catch (const CalendlyError& e) {
        print_error(format_error(e));
        throw;
    }
}

// --- Cancel handler ---

void handle_cancel(const std::string& uuid, const std::string& reason, bool yes) {
    try {
        // Skip confirmation if --yes flag or non-interactive (piped stdin)
        if (!yes && isatty(STDIN_FILENO)) {
            std::cerr << "Are you sure you want to cancel event " << short_uuid(uuid) << "? [y/N] ";
            std::string confirmation;
            std::getline(std::cin, confirmation);

            if (confirmation != "y" && confirmation != "Y" && confirmation != "yes" && confirmation != "Yes") {
                print_warning("Cancellation aborted.");
                return;
            }
        }

        CancellationRequest req;
        if (!reason.empty()) {
            req.reason = reason;
        }

        auto format = get_output_format();
        auto response = events_api::cancel(uuid, req);

        if (format == OutputFormat::Json) {
            json j;
            j["canceled_by"] = response.canceled_by;
            if (response.reason) { j["reason"] = *response.reason; }
            output_json(j, std::cout);
            return;
        }

        DetailRenderer detail;
        detail.add_field("Canceled By", response.canceled_by);
        detail.add_field("Reason", response.reason);
        detail.render(std::cout);

        print_success("Event cancelled successfully.");

    } catch (const CalendlyError& e) {
        print_error(format_error(e));
        throw;
    }
}

}  // anonymous namespace

// --- Command registration ---

void register_commands(CLI::App& app) {
    auto* events = app.add_subcommand("events", "Manage scheduled events");
    events->require_subcommand(1);

    // --- events list ---
    auto* list_cmd = events->add_subcommand("list", "List scheduled events");

    auto* status_opt = new std::string();
    auto* min_start_opt = new std::string();
    auto* max_start_opt = new std::string();
    auto* sort_opt = new std::string();
    auto* count_opt = new int(get_default_count());
    auto* all_opt = new bool(false);
    auto* today_opt = new bool(false);
    auto* tomorrow_opt = new bool(false);
    auto* this_week_opt = new bool(false);

    list_cmd->add_option("--status", *status_opt, "Filter by status (confirmed, cancelled)");
    list_cmd->add_option("--min-start", *min_start_opt, "Minimum start time (ISO 8601)");
    list_cmd->add_option("--max-start", *max_start_opt, "Maximum start time (ISO 8601)");
    list_cmd->add_option("--count", *count_opt, "Number of results per page (1-100)")
        ->default_val(20);
    list_cmd->add_flag("--all", *all_opt, "Fetch all pages");
    list_cmd->add_option("--sort", *sort_opt, "Sort order (e.g., start_time:asc)");
    list_cmd->add_flag("--today", *today_opt, "Show today's events");
    list_cmd->add_flag("--tomorrow", *tomorrow_opt, "Show tomorrow's events");
    list_cmd->add_flag("--this-week", *this_week_opt, "Show this week's remaining events");

    list_cmd->callback([=]() {
        handle_list(*status_opt, *min_start_opt, *max_start_opt, *count_opt,
                    *all_opt, *sort_opt, *today_opt, *tomorrow_opt, *this_week_opt);
    });

    // --- events show ---
    auto* show_cmd = events->add_subcommand("show", "Show event details");

    auto* show_uuid = new std::string();
    show_cmd->add_option("uuid", *show_uuid, "Event UUID")->required();

    show_cmd->callback([=]() {
        handle_show(*show_uuid);
    });

    // --- events create (hidden - user-facing is `calendly book`) ---
    auto* create_cmd = events->add_subcommand("create", "Create a scheduled event");
    create_cmd->group("");  // Hidden from help

    auto* create_event_type = new std::string();
    auto* create_start_time = new std::string();
    auto* create_name = new std::string();
    auto* create_email = new std::string();
    auto* create_timezone = new std::string();
    auto* create_location_kind = new std::string();
    auto* create_location = new std::string();
    auto* create_guests = new std::vector<std::string>();
    auto* create_qa = new std::vector<std::string>();
    auto* create_dry_run = new bool(false);

    create_cmd->add_option("--event-type", *create_event_type, "Event type URI")->required();
    create_cmd->add_option("--start-time", *create_start_time, "Start time (ISO 8601)")->required();
    create_cmd->add_option("--name", *create_name, "Invitee name")->required();
    create_cmd->add_option("--email", *create_email, "Invitee email")->required();
    create_cmd->add_option("--timezone", *create_timezone, "Invitee timezone")->required();
    create_cmd->add_option("--location-kind", *create_location_kind, "Location kind");
    create_cmd->add_option("--location", *create_location, "Location value");
    create_cmd->add_option("--guest", *create_guests, "Guest email (repeatable)");
    create_cmd->add_option("--qa", *create_qa, "Question::Answer pair (repeatable)");
    create_cmd->add_flag("--dry-run", *create_dry_run, "Show request body without sending");

    create_cmd->callback([=]() {
        handle_create(*create_event_type, *create_start_time, *create_name,
                      *create_email, *create_timezone, *create_location_kind,
                      *create_location, *create_guests, *create_qa, *create_dry_run);
    });

    // --- events cancel ---
    auto* cancel_cmd = events->add_subcommand("cancel", "Cancel a scheduled event");

    auto* cancel_uuid = new std::string();
    auto* cancel_reason = new std::string();
    auto* cancel_yes = new bool(false);

    cancel_cmd->add_option("uuid", *cancel_uuid, "Event UUID")->required();
    cancel_cmd->add_option("--reason", *cancel_reason, "Cancellation reason");
    cancel_cmd->add_flag("--yes,-y", *cancel_yes, "Skip confirmation prompt");

    cancel_cmd->callback([=]() {
        handle_cancel(*cancel_uuid, *cancel_reason, *cancel_yes);
    });
}

}  // namespace events_commands
