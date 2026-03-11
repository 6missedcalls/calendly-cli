#include "core/config.h"
#include <toml++/toml.hpp>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>

std::string config_dir() {
    const char* xdg = std::getenv("XDG_CONFIG_HOME");
    if (xdg != nullptr && xdg[0] != '\0') {
        std::string dir(xdg);
        if (dir.back() != '/') {
            dir += '/';
        }
        return dir + "calendly/";
    }

    const char* home = std::getenv("HOME");
    if (home == nullptr || home[0] == '\0') {
        throw std::runtime_error("HOME environment variable not set");
    }

    return std::string(home) + "/.config/calendly/";
}

std::string config_file_path() {
    return config_dir() + "config.toml";
}

bool config_exists() {
    return std::filesystem::exists(config_file_path());
}

CalendlyConfig load_config() {
    CalendlyConfig cfg;

    std::string path = config_file_path();
    if (!std::filesystem::exists(path)) {
        return cfg;
    }

    toml::table tbl;
    try {
        tbl = toml::parse_file(path);
    } catch (const toml::parse_error&) {
        return cfg;
    }

    if (auto token = tbl["token"].value<std::string>()) {
        cfg.token = *token;
    }

    if (auto tz = tbl["default_timezone"].value<std::string>()) {
        cfg.default_timezone = *tz;
    }

    if (auto display = tbl["display"].as_table()) {
        if (auto color_val = display->get("color")) {
            if (auto b = color_val->value<bool>()) {
                cfg.color_enabled = *b;
            }
        }
    }

    if (auto defaults = tbl["defaults"].as_table()) {
        if (auto count_val = defaults->get("count")) {
            if (auto n = count_val->value<int64_t>()) {
                cfg.default_count = static_cast<int>(*n);
            }
        }
    }

    return cfg;
}

void save_config(const CalendlyConfig& cfg) {
    std::string dir = config_dir();
    std::filesystem::create_directories(dir);

    toml::table tbl;

    if (cfg.token.has_value()) {
        tbl.insert("token", cfg.token.value());
    }

    if (cfg.default_timezone.has_value()) {
        tbl.insert("default_timezone", cfg.default_timezone.value());
    }

    toml::table display;
    display.insert("color", cfg.color_enabled);
    tbl.insert("display", display);

    toml::table defaults;
    defaults.insert("count", static_cast<int64_t>(cfg.default_count));
    tbl.insert("defaults", defaults);

    std::string path = config_file_path();
    std::ofstream out(path);
    if (!out.is_open()) {
        throw std::runtime_error("Failed to open config file for writing: " + path);
    }

    out << tbl;
    out.close();

    // Set permissions to 600 (owner read/write only) since file may contain token
    std::filesystem::permissions(
        path,
        std::filesystem::perms::owner_read | std::filesystem::perms::owner_write,
        std::filesystem::perm_options::replace
    );
}
