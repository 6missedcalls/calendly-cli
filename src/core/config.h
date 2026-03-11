#pragma once
#include <optional>
#include <string>

struct CalendlyConfig {
    std::optional<std::string> token;
    std::optional<std::string> default_timezone;  // Override for display
    bool color_enabled = true;
    int default_count = 20;                       // Default page size
};

CalendlyConfig load_config();
void save_config(const CalendlyConfig& cfg);
std::string config_file_path();  // ~/.config/calendly/config.toml
std::string config_dir();        // ~/.config/calendly/
bool config_exists();
