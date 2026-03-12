#include "modules/invitees/commands.h"
#include "modules/invitees/api.h"
#include "core/color.h"
#include "core/error.h"
#include "core/filter.h"
#include "core/output.h"
#include "core/paginator.h"
#include "core/rest_client.h"
#include "core/types.h"

namespace invitees_commands {

namespace {

void handle_list(const std::string& event_uuid, int count, bool all,
                 const std::string& status, const std::string& sort) {
    auto format = get_output_format();

    try {
        // Normalize British "cancelled" → API's "canceled"
        std::string normalized_status = (status == "cancelled") ? "canceled" : status;

        // JSON single page: pass through raw API response
        if (format == OutputFormat::Json && !all) {
            std::map<std::string, std::string> params;
            params["count"] = std::to_string(validate_count(count));
            if (!normalized_status.empty()) {
                params["status"] = normalized_status;
            }
            if (!sort.empty()) {
                params["sort"] = validate_sort(sort);
            }
            auto response = rest_get("scheduled_events/" + event_uuid + "/invitees", params);
            output_json(response, std::cout);
            return;
        }

        invitees_api::ListOptions opts;
        opts.event_uuid = event_uuid;
        opts.count = validate_count(count);
        if (!normalized_status.empty()) {
            opts.status = normalized_status;
        }
        if (!sort.empty()) {
            opts.sort = validate_sort(sort);
        }

        PaginationOptions page_opts;
        page_opts.count = opts.count;
        page_opts.fetch_all = all;

        CursorPaginator<Invitee> paginator(
            [&opts](const std::optional<std::string>& token) {
                auto page_options = opts;
                page_options.page_token = token;
                return invitees_api::list(page_options);
            },
            page_opts
        );

        // JSON --all: aggregate raw API pages to preserve all fields
        if (format == OutputFormat::Json && all) {
            json all_items = json::array();
            std::map<std::string, std::string> params;
            params["count"] = std::to_string(validate_count(count));
            if (!normalized_status.empty()) params["status"] = normalized_status;
            if (!sort.empty()) params["sort"] = validate_sort(sort);

            while (true) {
                auto response = rest_get("scheduled_events/" + event_uuid + "/invitees", params);
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

        auto invitees = all ? paginator.fetch_all()
                            : paginator.fetch_page().collection;

        if (format == OutputFormat::Csv) {
            output_csv_header({"name", "email", "status", "timezone", "created_at"});
            for (const auto& inv : invitees) {
                output_csv_row({
                    inv.name,
                    inv.email,
                    inv.status,
                    inv.timezone.value_or(""),
                    inv.created_at
                });
            }
            return;
        }

        // Table view
        TableRenderer table({
            {"NAME",       6, 25},
            {"EMAIL",      6, 30},
            {"STATUS",     4, 10},
            {"TIMEZONE",   4, 20},
            {"CREATED",    6, 20}
        });

        for (const auto& inv : invitees) {
            std::string status_str = color::invitee_status(inv.status, inv.status);

            table.add_row({
                inv.name,
                inv.email,
                status_str,
                inv.timezone.value_or(""),
                format_relative_time(inv.created_at)
            });
        }

        if (table.empty()) {
            print_warning("No invitees found for this event.");
            return;
        }

        table.render(std::cout);

    } catch (const CalendlyError& e) {
        print_error(format_error(e));
        throw;
    }
}

void handle_show(const std::string& invitee_uuid) {
    auto format = get_output_format();

    try {
        if (format == OutputFormat::Json) {
            auto response = rest_get("invitees/" + invitee_uuid);
            output_json(response, std::cout);
            return;
        }

        auto record = invitees_api::get(invitee_uuid);

        if (format == OutputFormat::Csv) {
            output_csv_header({"invitee_uuid", "status", "start_time", "end_time", "event", "created_at"});
            output_csv_row({
                record.invitee_uuid,
                record.status,
                record.start_time,
                record.end_time,
                record.event,
                record.created_at
            });
            return;
        }

        // Detail view — InviteRecord has 8 fields.
        // Do NOT show invitation_secret in detail view (sensitive token).
        DetailRenderer detail;
        detail.add_field("Invitee UUID", record.invitee_uuid);
        detail.add_field("Status", color::invitee_status(record.status, record.status));
        detail.add_field("Start Time", record.start_time);
        detail.add_field("End Time", record.end_time);
        detail.add_field("Event", strip_base_url(record.event));
        detail.add_field("Created", format_relative_time(record.created_at));
        detail.add_field("URI", record.uri);
        detail.render(std::cout);

    } catch (const CalendlyError& e) {
        print_error(format_error(e));
        throw;
    }
}

void handle_book(const std::string& event_type_uri, const std::string& email,
                 const std::string& name, const std::string& timezone,
                 const std::string& start_time, const std::string& first_name,
                 const std::string& last_name) {
    auto format = get_output_format();

    try {
        // Build CreateEventRequest shape (Resolution B-03)
        json invitee = json::object();
        invitee["email"] = email;
        if (!name.empty()) { invitee["name"] = name; }
        if (!timezone.empty()) { invitee["timezone"] = timezone; }
        if (!first_name.empty()) { invitee["first_name"] = first_name; }
        if (!last_name.empty()) { invitee["last_name"] = last_name; }

        json request_body = json::object();
        request_body["event_type"] = event_type_uri;
        request_body["start_time"] = start_time;
        request_body["invitee"] = invitee;

        if (format == OutputFormat::Json) {
            auto response = rest_post("scheduled_events", request_body);
            output_json(response, std::cout);
            return;
        }

        // POST to scheduled_events (same endpoint for all output formats)
        auto response = rest_post("scheduled_events", request_body);
        auto booking = response.get<BookingResponse>();

        if (format == OutputFormat::Csv) {
            output_csv_header({"uri", "email", "name", "status", "event", "cancel_url", "reschedule_url"});
            output_csv_row({
                booking.uri,
                booking.email,
                booking.name,
                booking.status,
                booking.event,
                booking.cancel_url,
                booking.reschedule_url
            });
            return;
        }

        // Detail view for booking confirmation
        DetailRenderer detail;
        detail.add_field("Name", booking.name);
        detail.add_field("Email", booking.email);
        detail.add_field("First Name", booking.first_name);
        detail.add_field("Last Name", booking.last_name);
        detail.add_field("Status", color::invitee_status(booking.status, booking.status));
        detail.add_field("Timezone", booking.timezone);
        detail.add_field("Event", strip_base_url(booking.event));

        if (!booking.questions_and_answers.empty()) {
            detail.add_blank_line();
            detail.add_section("Questions & Answers");
            for (size_t i = 0; i < booking.questions_and_answers.size(); ++i) {
                const auto& qa = booking.questions_and_answers[i];
                std::string prefix = "  [" + std::to_string(i + 1) + "] ";
                detail.add_field(prefix + "Q", qa.question);
                detail.add_field(prefix + "A", qa.answer);
            }
        }

        detail.add_blank_line();
        detail.add_field("Cancel URL", booking.cancel_url);
        detail.add_field("Reschedule URL", booking.reschedule_url);
        detail.add_field("Rescheduled", std::string(booking.rescheduled ? "yes" : "no"));
        if (booking.old_invitee.has_value()) {
            detail.add_field("Old Invitee", strip_base_url(booking.old_invitee.value()));
        }
        if (booking.new_invitee.has_value()) {
            detail.add_field("New Invitee", strip_base_url(booking.new_invitee.value()));
        }
        detail.add_field("Created", format_relative_time(booking.created_at));
        detail.add_field("Updated", format_relative_time(booking.updated_at));
        detail.add_field("URI", booking.uri);
        detail.render(std::cout);

        print_success("Booking created successfully.");

    } catch (const CalendlyError& e) {
        print_error(format_error(e));
        throw;
    }
}

}  // namespace

void register_commands(CLI::App& app) {
    auto* inv_group = app.add_subcommand("invitees", "Manage invitees");
    inv_group->require_subcommand(1);

    // --- list ---
    auto* list_cmd = inv_group->add_subcommand("list", "List invitees for a scheduled event");

    auto* list_event_uuid = new std::string();
    list_cmd->add_option("event_uuid", *list_event_uuid, "Scheduled event UUID")
        ->required();

    auto* list_count = new int(get_default_count());
    list_cmd->add_option("--count", *list_count, "Number of results per page (1-100)")
        ->default_val(20);

    auto* list_all = new bool(false);
    list_cmd->add_flag("--all", *list_all, "Fetch all pages");

    auto* list_status = new std::string();
    list_cmd->add_option("--status", *list_status, "Filter by status (active or canceled)");

    auto* list_sort = new std::string();
    list_cmd->add_option("--sort", *list_sort, "Sort field:direction (e.g. created_at:asc)");

    list_cmd->callback([list_event_uuid, list_count, list_all, list_status, list_sort]() {
        handle_list(*list_event_uuid, *list_count, *list_all, *list_status, *list_sort);
    });

    // --- show ---
    auto* show_cmd = inv_group->add_subcommand("show", "Show invitee details");

    auto* show_uuid = new std::string();
    show_cmd->add_option("invitee_uuid", *show_uuid, "Invitee UUID")
        ->required();

    show_cmd->callback([show_uuid]() {
        handle_show(*show_uuid);
    });

    // --- book (hidden — user-facing is `calendly book`) ---
    auto* book_cmd = inv_group->add_subcommand("book", "Book an invitee");
    book_cmd->group("");  // Hidden subcommand

    auto* book_event_type_uri = new std::string();
    book_cmd->add_option("--event-type", *book_event_type_uri, "Event type URI")
        ->required();

    auto* book_start_time = new std::string();
    book_cmd->add_option("--start-time", *book_start_time, "Start time (ISO 8601)")
        ->required();

    auto* book_email = new std::string();
    book_cmd->add_option("--email", *book_email, "Invitee email address")
        ->required();

    auto* book_name = new std::string();
    book_cmd->add_option("--name", *book_name, "Invitee full name");

    auto* book_timezone = new std::string();
    book_cmd->add_option("--timezone", *book_timezone, "Invitee timezone (IANA)");

    auto* book_first_name = new std::string();
    book_cmd->add_option("--first-name", *book_first_name, "Invitee first name");

    auto* book_last_name = new std::string();
    book_cmd->add_option("--last-name", *book_last_name, "Invitee last name");

    book_cmd->callback([book_event_type_uri, book_start_time, book_email, book_name,
                        book_timezone, book_first_name, book_last_name]() {
        handle_book(*book_event_type_uri, *book_email, *book_name,
                    *book_timezone, *book_start_time, *book_first_name,
                    *book_last_name);
    });
}

}  // namespace invitees_commands
