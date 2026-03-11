#include "modules/cache/commands.h"
#include "core/cache.h"
#include "core/output.h"
#include <iostream>

namespace cache_commands {

namespace {

void handle_clear() {
    Cache cache;
    cache.clear_all();
    print_success("Cache cleared.");
}

void handle_show() {
    auto format = get_output_format();
    Cache cache;
    auto s = cache.status();

    if (format == OutputFormat::Json) {
        nlohmann::json j = nlohmann::json::object();
        j["cache_dir"] = s.cache_path;
        j["user"]["cached"] = s.user_cached;
        j["user"]["cached_at"] = s.user_cached_at;
        j["event_types"]["cached"] = s.event_types_cached;
        j["event_types"]["cached_at"] = s.event_types_cached_at;
        j["event_types"]["count"] = s.event_types_count;
        output_json(j, std::cout);
        return;
    }

    DetailRenderer detail;
    detail.add_section("Cache Status");
    detail.add_field("Cache Directory", s.cache_path);
    detail.add_blank_line();
    detail.add_field("User Profile",
        std::string(s.user_cached ? "cached" : "not cached") +
        (s.user_cached ? " (since " + s.user_cached_at + ")" : ""));
    detail.add_field("Event Types",
        std::string(s.event_types_cached ? "cached" : "not cached") +
        (s.event_types_cached
            ? " (" + std::to_string(s.event_types_count) + " types, since " + s.event_types_cached_at + ")"
            : ""));
    detail.render(std::cout);
}

}  // namespace

void register_commands(CLI::App& app) {
    auto* cache_group = app.add_subcommand("cache", "Manage local cache");
    cache_group->require_subcommand(1);

    // --- clear ---
    auto* clear_cmd = cache_group->add_subcommand("clear", "Clear all cached data");
    clear_cmd->callback(handle_clear);

    // --- show ---
    auto* show_cmd = cache_group->add_subcommand("show", "Show cache status");
    show_cmd->callback(handle_show);
}

}  // namespace cache_commands
