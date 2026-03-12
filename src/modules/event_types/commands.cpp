#include "modules/event_types/commands.h"
#include "modules/event_types/api.h"
#include "core/color.h"
#include "core/error.h"
#include "core/filter.h"
#include "core/paginator.h"
#include "core/rest_client.h"
#include "core/output.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace event_types_commands {

namespace {

std::string get_current_user_uri() {
    auto response = rest_get("users/me");
    return response["resource"]["uri"].get<std::string>();
}

std::string now_iso8601() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm utc_tm{};
    gmtime_r(&time_t_now, &utc_tm);
    std::ostringstream oss;
    oss << std::put_time(&utc_tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

std::string date_start_iso8601(const std::string& date) {
    return date + "T00:00:00Z";
}

std::string date_end_iso8601(const std::string& date) {
    return date + "T23:59:59Z";
}

std::string today_date() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm utc_tm{};
    gmtime_r(&time_t_now, &utc_tm);
    std::ostringstream oss;
    oss << std::put_time(&utc_tm, "%Y-%m-%d");
    return oss.str();
}

std::string tomorrow_date() {
    auto now = std::chrono::system_clock::now();
    auto tomorrow = now + std::chrono::hours(24);
    auto time_t_val = std::chrono::system_clock::to_time_t(tomorrow);
    std::tm utc_tm{};
    gmtime_r(&time_t_val, &utc_tm);
    std::ostringstream oss;
    oss << std::put_time(&utc_tm, "%Y-%m-%d");
    return oss.str();
}

std::string end_of_week_date() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm utc_tm{};
    gmtime_r(&time_t_now, &utc_tm);
    int wday = utc_tm.tm_wday;  // 0=Sunday
    int days_until_sunday = (wday == 0) ? 0 : (7 - wday);
    auto end_of_week = now + std::chrono::hours(24 * days_until_sunday);
    auto time_t_end = std::chrono::system_clock::to_time_t(end_of_week);
    std::tm end_tm{};
    gmtime_r(&time_t_end, &end_tm);
    std::ostringstream oss;
    oss << std::put_time(&end_tm, "%Y-%m-%d");
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

void handle_list(bool active_flag, bool inactive_flag, int count, bool all,
                 const std::string& sort) {
    auto format = get_output_format();

    try {
        auto user_uri = get_current_user_uri();

        auto validated_count = validate_count(count);

        // JSON single page: pass through raw API response
        if (format == OutputFormat::Json && !all) {
            std::map<std::string, std::string> params;
            params["user"] = user_uri;
            params["count"] = std::to_string(validated_count);
            if (active_flag) {
                params["active"] = "true";
            } else if (inactive_flag) {
                params["active"] = "false";
            }
            if (!sort.empty()) {
                params["sort"] = validate_sort(sort);
            }
            auto response = rest_get("event_types", params);
            output_json(response, std::cout);
            return;
        }

        event_types_api::ListOptions opts;
        opts.user_uri = user_uri;
        opts.count = validated_count;
        if (active_flag) {
            opts.active = true;
        } else if (inactive_flag) {
            opts.active = false;
        }
        if (!sort.empty()) {
            opts.sort = validate_sort(sort);
        }

        PaginationOptions page_opts;
        page_opts.count = opts.count;
        page_opts.fetch_all = all;

        CursorPaginator<EventType> paginator(
            [&opts](const std::optional<std::string>& token) {
                auto page_options = opts;
                page_options.page_token = token;
                return event_types_api::list(page_options);
            },
            page_opts
        );

        // JSON --all: aggregate raw API pages to preserve all fields
        if (format == OutputFormat::Json && all) {
            json all_items = json::array();
            std::map<std::string, std::string> params;
            params["user"] = user_uri;
            params["count"] = std::to_string(validated_count);
            if (active_flag) params["active"] = "true";
            else if (inactive_flag) params["active"] = "false";
            if (!sort.empty()) params["sort"] = validate_sort(sort);

            while (true) {
                auto response = rest_get("event_types", params);
                if (response.contains("collection") && response["collection"].is_array()) {
                    for (auto& item : response["collection"]) {
                        all_items.push_back(std::move(item));
                    }
                }
                if (response.contains("pagination")
                    && response["pagination"].contains("next_page_token")
                    && !response["pagination"]["next_page_token"].is_null()) {
                    params["page_token"] = response["pagination"]["next_page_token"].get<std::string>();
                } else {
                    break;
                }
            }
            json result;
            result["collection"] = all_items;
            result["total"] = all_items.size();
            output_json(result, std::cout);
            return;
        }

        auto event_types = all ? paginator.fetch_all()
                               : paginator.fetch_page().collection;

        if (format == OutputFormat::Csv) {
            output_csv_header({"name", "duration", "kind", "active", "slug", "scheduling_url"});
            for (const auto& et : event_types) {
                output_csv_row({
                    et.name,
                    std::to_string(et.duration),
                    et.kind,
                    et.active ? "true" : "false",
                    et.slug,
                    et.scheduling_url
                });
            }
            return;
        }

        // Table view
        TableRenderer table({
            {"NAME",           8, 30},
            {"DURATION",       4, 10, true},
            {"KIND",           4, 8},
            {"ACTIVE",         4, 8},
            {"SLUG",           4, 20},
            {"SCHEDULING URL", 8, 50}
        });

        for (const auto& et : event_types) {
            std::string duration_str = std::to_string(et.duration) + "m";
            std::string active_str = et.active
                ? color::green("yes")
                : color::dim("no");

            table.add_row({
                et.name,
                duration_str,
                et.kind,
                active_str,
                et.slug,
                et.scheduling_url
            });
        }

        if (table.empty()) {
            print_warning("No event types found.");
            return;
        }

        table.render(std::cout);

    } catch (const CalendlyError& e) {
        print_error(format_error(e));
        throw;
    }
}

void handle_show(const std::string& uuid) {
    auto format = get_output_format();

    try {
        if (format == OutputFormat::Json) {
            auto response = rest_get("event_types/" + uuid);
            output_json(response, std::cout);
            return;
        }

        auto et = event_types_api::get(uuid);

        if (format == OutputFormat::Csv) {
            output_csv_header({"name", "duration", "kind", "active", "slug", "scheduling_url"});
            output_csv_row({
                et.name,
                std::to_string(et.duration),
                et.kind,
                et.active ? "true" : "false",
                et.slug,
                et.scheduling_url
            });
            return;
        }

        // Detail view
        DetailRenderer detail;
        detail.add_field("Name", et.name);
        detail.add_field("Slug", et.slug);
        detail.add_field("Active", et.active ? color::green("yes") : color::red("no"));
        detail.add_field("Duration", std::to_string(et.duration) + " minutes");
        if (!et.duration_options.empty()) {
            std::string opts_str;
            for (size_t i = 0; i < et.duration_options.size(); ++i) {
                if (i > 0) opts_str += ", ";
                opts_str += std::to_string(et.duration_options[i]) + "m";
            }
            detail.add_field("Duration Options", opts_str);
        }
        detail.add_field("Kind", et.kind);
        detail.add_field("Type", et.type);
        detail.add_field("Color", color::from_hex(et.color, et.color));
        detail.add_field("Scheduling URL", et.scheduling_url);
        detail.add_field("Booking Method", et.booking_method);
        detail.add_field("Pooling Type", et.pooling_type);
        detail.add_field("Secret", std::string(et.secret ? "yes" : "no"));
        detail.add_field("Admin Managed", std::string(et.admin_managed ? "yes" : "no"));
        if (et.is_paid.has_value()) {
            detail.add_field("Paid", std::string(et.is_paid.value() ? "yes" : "no"));
        }
        detail.add_field("Description", et.description_plain);
        detail.add_field("Internal Note", et.internal_note);

        detail.add_blank_line();
        detail.add_section("Profile");
        detail.add_field("Profile Type", et.profile.type);
        detail.add_field("Profile Name", et.profile.name);
        detail.add_field("Profile Owner", strip_base_url(et.profile.owner));

        if (!et.locations.empty()) {
            detail.add_blank_line();
            detail.add_section("Locations");
            for (size_t i = 0; i < et.locations.size(); ++i) {
                const auto& loc = et.locations[i];
                std::string prefix = "  [" + std::to_string(i + 1) + "] ";
                detail.add_field(prefix + "Kind", loc.kind);
                detail.add_field(prefix + "Location", loc.location);
                detail.add_field(prefix + "Phone", loc.phone_number);
                detail.add_field(prefix + "Info", loc.additional_info);
            }
        }

        if (!et.custom_questions.empty()) {
            detail.add_blank_line();
            detail.add_section("Custom Questions");
            for (size_t i = 0; i < et.custom_questions.size(); ++i) {
                const auto& q = et.custom_questions[i];
                std::string prefix = "  [" + std::to_string(i + 1) + "] ";
                detail.add_field(prefix + "Question", q.name);
                detail.add_field(prefix + "Type", q.type);
                detail.add_field(prefix + "Position", std::to_string(q.position));
                detail.add_field(prefix + "Enabled", std::string(q.enabled ? "yes" : "no"));
                detail.add_field(prefix + "Required", std::string(q.required ? "yes" : "no"));
                if (!q.answer_choices.empty()) {
                    std::string choices;
                    for (size_t ci = 0; ci < q.answer_choices.size(); ++ci) {
                        if (ci > 0) choices += ", ";
                        choices += q.answer_choices[ci];
                    }
                    detail.add_field(prefix + "Choices", choices);
                }
                detail.add_field(prefix + "Include Other", std::string(q.include_other ? "yes" : "no"));
            }
        }

        detail.add_blank_line();
        detail.add_field("Created", format_relative_time(et.created_at));
        detail.add_field("Updated", format_relative_time(et.updated_at));
        if (et.deleted_at.has_value()) {
            detail.add_field("Deleted", format_relative_time(et.deleted_at.value()));
        }
        detail.add_field("URI", et.uri);

        detail.render(std::cout);

    } catch (const CalendlyError& e) {
        print_error(format_error(e));
        throw;
    }
}

void handle_available_times(const std::string& uuid,
                            const std::string& start_opt,
                            const std::string& end_opt,
                            bool today_flag,
                            bool tomorrow_flag,
                            bool this_week_flag,
                            const std::string& date_opt) {
    auto format = get_output_format();

    try {
        std::string event_type_uri = build_uri("event_types", uuid);
        std::string start_time;
        std::string end_time;

        // Smart time defaults
        if (!start_opt.empty() && !end_opt.empty()) {
            start_time = start_opt;
            end_time = end_opt;
        } else if (today_flag) {
            start_time = now_iso8601();
            end_time = date_end_iso8601(today_date());
        } else if (tomorrow_flag) {
            auto tmrw = tomorrow_date();
            start_time = date_start_iso8601(tmrw);
            end_time = date_end_iso8601(tmrw);
        } else if (this_week_flag) {
            start_time = now_iso8601();
            end_time = date_end_iso8601(end_of_week_date());
        } else if (!date_opt.empty()) {
            start_time = date_start_iso8601(date_opt);
            end_time = date_end_iso8601(date_opt);
        } else {
            // Default: now to +7 days (max window)
            start_time = now_iso8601();
            end_time = add_days_iso8601(7);
        }

        if (format == OutputFormat::Json) {
            std::map<std::string, std::string> params;
            params["event_type"] = event_type_uri;
            params["start_time"] = start_time;
            params["end_time"] = end_time;
            auto response = rest_get("event_type_available_times", params);
            output_json(response, std::cout);
            return;
        }

        event_types_api::AvailableTimesOptions opts;
        opts.event_type_uri = event_type_uri;
        opts.start_time = start_time;
        opts.end_time = end_time;

        auto times = event_types_api::available_times(opts);

        if (format == OutputFormat::Csv) {
            output_csv_header({"status", "start_time", "invitees_remaining"});
            for (const auto& t : times) {
                output_csv_row({
                    t.status,
                    t.start_time,
                    std::to_string(t.invitees_remaining)
                });
            }
            return;
        }

        // Table view
        TableRenderer table({
            {"START TIME",          10, 30},
            {"INVITEES REMAINING",   4, 20, true},
            {"STATUS",               4, 12}
        });

        for (const auto& t : times) {
            std::string status_str = (t.status == "available")
                ? color::green(t.status)
                : color::dim(t.status);

            table.add_row({
                t.start_time,
                std::to_string(t.invitees_remaining),
                status_str
            });
        }

        if (table.empty()) {
            print_warning("No available times found for the specified range.");
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
    auto* et_group = app.add_subcommand("event-types", "Manage event types");
    et_group->require_subcommand(1);

    // --- list ---
    auto* list_cmd = et_group->add_subcommand("list", "List your event types");

    auto* active_flag = list_cmd->add_flag("--active", "Show only active event types");
    auto* inactive_flag = list_cmd->add_flag("--inactive", "Show only inactive event types");
    active_flag->excludes(inactive_flag);
    inactive_flag->excludes(active_flag);

    auto* count_opt = new int(get_default_count());
    list_cmd->add_option("--count", *count_opt, "Number of results per page (1-100)")
        ->default_val(20);

    auto* all_flag_val = new bool(false);
    list_cmd->add_flag("--all", *all_flag_val, "Fetch all pages");

    auto* sort_opt = new std::string();
    list_cmd->add_option("--sort", *sort_opt, "Sort field:direction (e.g. name:asc)");

    list_cmd->callback([active_flag, inactive_flag, count_opt, all_flag_val, sort_opt]() {
        handle_list(
            active_flag->count() > 0,
            inactive_flag->count() > 0,
            *count_opt,
            *all_flag_val,
            *sort_opt
        );
    });

    // --- show ---
    auto* show_cmd = et_group->add_subcommand("show", "Show event type details");

    auto* show_uuid = new std::string();
    show_cmd->add_option("uuid", *show_uuid, "Event type UUID")
        ->required();

    show_cmd->callback([show_uuid]() {
        handle_show(*show_uuid);
    });

    // --- available-times ---
    auto* avail_cmd = et_group->add_subcommand("available-times", "List available times for an event type");

    auto* avail_uuid = new std::string();
    avail_cmd->add_option("uuid", *avail_uuid, "Event type UUID")
        ->required();

    auto* start_opt = new std::string();
    avail_cmd->add_option("--start", *start_opt, "Start time (ISO 8601)");

    auto* end_opt = new std::string();
    avail_cmd->add_option("--end", *end_opt, "End time (ISO 8601)");

    auto* today_flag_val = new bool(false);
    avail_cmd->add_flag("--today", *today_flag_val, "Show times for today");

    auto* tomorrow_flag_val = new bool(false);
    avail_cmd->add_flag("--tomorrow", *tomorrow_flag_val, "Show times for tomorrow");

    auto* this_week_flag_val = new bool(false);
    avail_cmd->add_flag("--this-week", *this_week_flag_val, "Show times for the rest of this week");

    auto* date_opt = new std::string();
    avail_cmd->add_option("--date", *date_opt, "Show times for a specific date (YYYY-MM-DD)");

    avail_cmd->callback([avail_uuid, start_opt, end_opt, today_flag_val,
                         tomorrow_flag_val, this_week_flag_val, date_opt]() {
        handle_available_times(
            *avail_uuid,
            *start_opt,
            *end_opt,
            *today_flag_val,
            *tomorrow_flag_val,
            *this_week_flag_val,
            *date_opt
        );
    });
}

}  // namespace event_types_commands
