#pragma once
#include <optional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "modules/users/model.h"
#include "modules/event_types/model.h"

using json = nlohmann::json;

struct CacheEntry {
    json data;
    std::string cached_at;       // ISO 8601 timestamp
    int ttl_seconds = 0;
};

inline void to_json(json& j, const CacheEntry& e) {
    j = json{{"data", e.data}, {"cached_at", e.cached_at}, {"ttl_seconds", e.ttl_seconds}};
}

inline void from_json(const json& j, CacheEntry& e) {
    e.data = j.value("data", json{});
    e.cached_at = j.value("cached_at", "");
    e.ttl_seconds = j.value("ttl_seconds", 0);
}

// Global no-cache bypass flag (set by --no-cache)
[[nodiscard]] bool is_cache_disabled() noexcept;
void set_cache_disabled(bool disabled) noexcept;

class Cache {
public:
    Cache();

    // User profile (24h TTL)
    std::optional<User> get_user();
    void set_user(const User& user);

    // Event types (1h TTL)
    std::optional<std::vector<EventType>> get_event_types();
    void set_event_types(const std::vector<EventType>& types);

    // Invalidation
    void clear_all();
    void clear_user();
    void clear_event_types();

    // Check if entry is fresh
    bool is_fresh(const std::string& key) const;

    // Cache directory path
    std::string cache_dir() const;

    // Cache status info (for `cache show`)
    struct CacheStatus {
        bool user_cached = false;
        std::string user_cached_at;
        bool event_types_cached = false;
        std::string event_types_cached_at;
        int event_types_count = 0;
        std::string cache_path;
    };
    CacheStatus status() const;

private:
    std::string cache_dir_;

    static constexpr int USER_TTL_SECONDS = 24 * 60 * 60;         // 24 hours
    static constexpr int EVENT_TYPES_TTL_SECONDS = 60 * 60;       // 1 hour

    json read_cache_file(const std::string& filename) const;
    void write_cache_file(const std::string& filename, const json& data) const;
    bool is_entry_fresh(const CacheEntry& entry) const;
    static std::string now_iso8601();
};
