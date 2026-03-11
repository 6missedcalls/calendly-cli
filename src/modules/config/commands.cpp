#include "modules/config/commands.h"
#include "core/config.h"
#include "core/color.h"
#include "core/output.h"
#include <iostream>

namespace config_commands {

namespace {

std::string mask_token(const std::optional<std::string>& token) {
    if (!token.has_value() || token->empty()) {
        return "(not set)";
    }
    const auto& val = *token;
    if (val.size() <= 4) {
        return val + "...";
    }
    return val.substr(0, 4) + "...";
}

void handle_show() {
    auto format = get_output_format();
    auto cfg = load_config();

    if (format == OutputFormat::Json) {
        nlohmann::json j = nlohmann::json::object();
        j["token"] = mask_token(cfg.token);
        j["default_timezone"] = cfg.default_timezone.value_or("");
        j["color_enabled"] = cfg.color_enabled;
        j["default_count"] = cfg.default_count;
        output_json(j, std::cout);
        return;
    }

    DetailRenderer detail;
    detail.add_field("Token", mask_token(cfg.token));
    detail.add_field("Default Timezone", cfg.default_timezone.value_or("(not set)"));
    detail.add_field("Color Enabled", std::string(cfg.color_enabled ? "true" : "false"));
    detail.add_field("Default Count", std::to_string(cfg.default_count));
    detail.add_field("Config File", config_file_path());
    detail.render(std::cout);
}

void handle_set(const std::string& key, const std::string& value) {
    auto cfg = load_config();

    if (key == "token") {
        auto updated = CalendlyConfig{
            value,
            cfg.default_timezone,
            cfg.color_enabled,
            cfg.default_count
        };
        save_config(updated);
        print_success("Token updated.");
    } else if (key == "timezone") {
        auto updated = CalendlyConfig{
            cfg.token,
            value,
            cfg.color_enabled,
            cfg.default_count
        };
        save_config(updated);
        print_success("Default timezone set to: " + value);
    } else if (key == "color") {
        bool color_val = (value == "true" || value == "1" || value == "yes");
        auto updated = CalendlyConfig{
            cfg.token,
            cfg.default_timezone,
            color_val,
            cfg.default_count
        };
        save_config(updated);
        print_success("Color " + std::string(color_val ? "enabled" : "disabled") + ".");
    } else if (key == "count") {
        int count_val = 0;
        try {
            count_val = std::stoi(value);
        } catch (...) {
            print_error("Invalid count value: " + value + " (must be an integer)");
            return;
        }
        if (count_val < 1 || count_val > 100) {
            print_error("Count must be between 1 and 100.");
            return;
        }
        auto updated = CalendlyConfig{
            cfg.token,
            cfg.default_timezone,
            cfg.color_enabled,
            count_val
        };
        save_config(updated);
        print_success("Default count set to: " + std::to_string(count_val));
    } else {
        print_error("Unknown config key: " + key + ". Valid keys: token, timezone, color, count");
    }
}

void handle_path() {
    std::cout << config_file_path() << "\n";
}

}  // namespace

void register_commands(CLI::App& app) {
    auto* cfg_group = app.add_subcommand("config", "Manage CLI configuration");
    cfg_group->require_subcommand(1);

    // --- show ---
    auto* show_cmd = cfg_group->add_subcommand("show", "Display current configuration");
    show_cmd->callback(handle_show);

    // --- set ---
    auto* set_cmd = cfg_group->add_subcommand("set", "Set a configuration value");

    auto* key_val = new std::string();
    set_cmd->add_option("key", *key_val, "Config key (token, timezone, color, count)")
        ->required();

    auto* value_val = new std::string();
    set_cmd->add_option("value", *value_val, "Config value")
        ->required();

    set_cmd->callback([key_val, value_val]() {
        handle_set(*key_val, *value_val);
    });

    // --- path ---
    auto* path_cmd = cfg_group->add_subcommand("path", "Print the config file path");
    path_cmd->callback(handle_path);
}

}  // namespace config_commands
