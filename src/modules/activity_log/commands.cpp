#include "modules/activity_log/commands.h"
#include "modules/activity_log/api.h"
#include "core/color.h"
#include "core/error.h"
#include "core/filter.h"
#include "core/output.h"
#include "core/rest_client.h"

namespace activity_log_commands {

namespace {

std::string get_current_user_org_uri() {
    auto response = rest_get("users/me");
    return response["resource"]["current_organization"].get<std::string>();
}

std::string summarize_details(const json& details, size_t max_len = 40) {
    if (details.is_null() || details.empty()) {
        return "";
    }
    auto dump = details.dump();
    if (dump.size() <= max_len) {
        return dump;
    }
    return dump.substr(0, max_len - 3) + "...";
}

void handle_list(int count, bool all, const std::string& action,
                 const std::string& actor, const std::string& namespace_filter,
                 const std::string& min_occurred_at,
                 const std::string& max_occurred_at,
                 const std::string& sort) {
    auto format = get_output_format();

    try {
        auto org_uri = get_current_user_org_uri();

        // JSON single page: pass through raw API response
        if (format == OutputFormat::Json && !all) {
            std::map<std::string, std::string> params;
            params["organization"] = org_uri;
            params["count"] = std::to_string(validate_count(count));
            if (!action.empty()) {
                params["action"] = action;
            }
            if (!actor.empty()) {
                params["actor"] = actor;
            }
            if (!namespace_filter.empty()) {
                params["namespace"] = namespace_filter;
            }
            if (!min_occurred_at.empty()) {
                params["min_occurred_at"] = min_occurred_at;
            }
            if (!max_occurred_at.empty()) {
                params["max_occurred_at"] = max_occurred_at;
            }
            if (!sort.empty()) {
                params["sort"] = validate_sort(sort);
            }
            auto response = rest_get("activity_log_entries", params);
            output_json(response, std::cout);
            return;
        }

        // JSON --all: aggregate raw API pages to preserve all fields
        if (format == OutputFormat::Json && all) {
            json all_items = json::array();
            std::map<std::string, std::string> params;
            params["organization"] = org_uri;
            params["count"] = std::to_string(validate_count(count));
            if (!action.empty()) params["action"] = action;
            if (!actor.empty()) params["actor"] = actor;
            if (!namespace_filter.empty()) params["namespace"] = namespace_filter;
            if (!min_occurred_at.empty()) params["min_occurred_at"] = min_occurred_at;
            if (!max_occurred_at.empty()) params["max_occurred_at"] = max_occurred_at;
            if (!sort.empty()) params["sort"] = validate_sort(sort);

            while (true) {
                auto response = rest_get("activity_log_entries", params);
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

        activity_log_api::ListOptions opts;
        opts.organization = org_uri;
        opts.count = validate_count(count);
        if (!action.empty()) {
            opts.action = action;
        }
        if (!actor.empty()) {
            opts.actor_uri = actor;
        }
        if (!namespace_filter.empty()) {
            opts.namespace_val = namespace_filter;
        }
        if (!min_occurred_at.empty()) {
            opts.min_occurred_at = min_occurred_at;
        }
        if (!max_occurred_at.empty()) {
            opts.max_occurred_at = max_occurred_at;
        }
        if (!sort.empty()) {
            opts.sort = validate_sort(sort);
        }

        std::vector<ActivityLogEntry> entries;
        int total_count = 0;
        bool exceeds_max = false;

        if (all) {
            std::optional<std::string> token = std::nullopt;
            while (true) {
                auto page_opts = opts;
                page_opts.page_token = token;
                auto resp = activity_log_api::list(page_opts);
                for (auto& entry : resp.collection) {
                    entries.push_back(std::move(entry));
                }
                if (!resp.pagination.next_page_token.has_value()) {
                    break;
                }
                token = resp.pagination.next_page_token;
            }
        } else {
            auto resp = activity_log_api::list(opts);
            entries = std::move(resp.collection);
            total_count = resp.total_count;
            exceeds_max = resp.exceeds_max_total_count;
        }

        if (format == OutputFormat::Csv) {
            output_csv_header({"occurred_at", "action", "actor", "details"});
            for (const auto& entry : entries) {
                output_csv_row({
                    entry.occurred_at,
                    entry.fully_qualified_name,
                    entry.actor.display_name,
                    summarize_details(entry.details)
                });
            }
            return;
        }

        TableRenderer table({
            {"OCCURRED",  8, 20},
            {"ACTION",    6, 30},
            {"ACTOR",     6, 25},
            {"DETAILS",   6, 40}
        });

        for (const auto& entry : entries) {
            table.add_row({
                format_relative_time(entry.occurred_at),
                entry.fully_qualified_name,
                entry.actor.display_name,
                summarize_details(entry.details)
            });
        }

        if (table.empty()) {
            print_warning("No activity log entries found.");
            return;
        }

        table.render(std::cout);

        if (total_count > 0) {
            auto total_msg = "Total: " + std::to_string(total_count) + " entries";
            if (exceeds_max) {
                total_msg += " (count exceeds maximum trackable total)";
            }
            std::cout << "\n" << color::dim(total_msg) << "\n";
        }

    } catch (const CalendlyError& e) {
        print_error(format_error(e));
        throw;
    }
}

}  // namespace

void register_commands(CLI::App& app) {
    auto* al_group = app.add_subcommand("activity-log", "View organization activity log");
    al_group->require_subcommand(1);

    // --- list ---
    auto* list_cmd = al_group->add_subcommand("list", "List activity log entries");

    auto* count_opt = new int(get_default_count());
    list_cmd->add_option("--count", *count_opt, "Number of results per page (1-100)")
        ->default_val(20);

    auto* all_flag_val = new bool(false);
    list_cmd->add_flag("--all", *all_flag_val, "Fetch all pages");

    auto* action_opt = new std::string();
    list_cmd->add_option("--action", *action_opt, "Filter by action (e.g. Add, Remove)");

    auto* actor_opt = new std::string();
    list_cmd->add_option("--actor", *actor_opt, "Filter by actor URI");

    auto* namespace_opt = new std::string();
    list_cmd->add_option("--namespace", *namespace_opt, "Filter by namespace (e.g. User, EventType)");

    auto* min_occurred_opt = new std::string();
    list_cmd->add_option("--min-occurred-at", *min_occurred_opt, "Minimum occurred_at (ISO 8601)");

    auto* max_occurred_opt = new std::string();
    list_cmd->add_option("--max-occurred-at", *max_occurred_opt, "Maximum occurred_at (ISO 8601)");

    auto* sort_opt = new std::string();
    list_cmd->add_option("--sort", *sort_opt, "Sort field:direction (e.g. occurred_at:desc)");

    list_cmd->callback([count_opt, all_flag_val, action_opt, actor_opt,
                        namespace_opt, min_occurred_opt, max_occurred_opt, sort_opt]() {
        handle_list(
            *count_opt,
            *all_flag_val,
            *action_opt,
            *actor_opt,
            *namespace_opt,
            *min_occurred_opt,
            *max_occurred_opt,
            *sort_opt
        );
    });
}

}  // namespace activity_log_commands
