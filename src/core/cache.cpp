#include "core/cache.h"
#include "core/output.h"
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

static bool g_cache_disabled = false;

bool is_cache_disabled() noexcept { return g_cache_disabled; }
void set_cache_disabled(bool disabled) noexcept { g_cache_disabled = disabled; }

Cache::Cache() {
    const char* xdg = std::getenv("XDG_CACHE_HOME");
    if (xdg != nullptr && xdg[0] != '\0') {
        std::string dir(xdg);
        if (dir.back() != '/') {
            dir += '/';
        }
        cache_dir_ = dir + "calendly/";
    } else {
        const char* home = std::getenv("HOME");
        if (home == nullptr || home[0] == '\0') {
            cache_dir_ = "/tmp/calendly-cache/";
        } else {
            cache_dir_ = std::string(home) + "/.cache/calendly/";
        }
    }
}

std::string Cache::now_iso8601() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm utc_tm{};
    gmtime_r(&time_t_now, &utc_tm);
    std::ostringstream oss;
    oss << std::put_time(&utc_tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

bool Cache::is_entry_fresh(const CacheEntry& entry) const {
    if (entry.cached_at.empty() || entry.ttl_seconds <= 0) {
        return false;
    }

    // Parse cached_at ISO 8601
    std::tm tm{};
    std::istringstream iss(entry.cached_at);
    iss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    if (iss.fail()) {
        return false;
    }

    auto cached_time = std::chrono::system_clock::from_time_t(timegm(&tm));
    auto now = std::chrono::system_clock::now();
    auto age = std::chrono::duration_cast<std::chrono::seconds>(now - cached_time);

    return age.count() < entry.ttl_seconds;
}

json Cache::read_cache_file(const std::string& filename) const {
    std::string path = cache_dir_ + filename;
    if (!std::filesystem::exists(path)) {
        return json{};
    }

    std::ifstream in(path);
    if (!in.is_open()) {
        return json{};
    }

    try {
        return json::parse(in);
    } catch (...) {
        return json{};
    }
}

void Cache::write_cache_file(const std::string& filename, const json& data) const {
    std::filesystem::create_directories(cache_dir_);

    // Set cache directory permissions to 700 (owner only)
    std::filesystem::permissions(
        cache_dir_,
        std::filesystem::perms::owner_all,
        std::filesystem::perm_options::replace
    );

    std::string path = cache_dir_ + filename;
    std::ofstream out(path);
    if (!out.is_open()) {
        print_verbose("Cache: failed to write " + path);
        return;
    }

    out << data.dump(2);
    out.close();

    // Set file permissions to 600 (owner read/write only)
    std::filesystem::permissions(
        path,
        std::filesystem::perms::owner_read | std::filesystem::perms::owner_write,
        std::filesystem::perm_options::replace
    );
}

// --- User cache ---

std::optional<User> Cache::get_user() {
    if (is_cache_disabled()) {
        print_verbose("Cache: bypassed (--no-cache)");
        return std::nullopt;
    }

    auto file_data = read_cache_file("user.json");
    if (file_data.is_null() || !file_data.is_object()) {
        return std::nullopt;
    }

    CacheEntry entry = file_data.get<CacheEntry>();
    if (!is_entry_fresh(entry)) {
        print_verbose("Cache: user cache expired");
        return std::nullopt;
    }

    print_verbose("Cache: user cache hit");
    return entry.data.get<User>();
}

void Cache::set_user(const User& user) {
    json user_json;
    user_json["uri"] = user.uri;
    user_json["name"] = user.name;
    user_json["slug"] = user.slug;
    user_json["email"] = user.email;
    user_json["scheduling_url"] = user.scheduling_url;
    user_json["timezone"] = user.timezone;
    user_json["time_notation"] = user.time_notation;
    if (user.avatar_url.has_value()) {
        user_json["avatar_url"] = user.avatar_url.value();
    } else {
        user_json["avatar_url"] = nullptr;
    }
    user_json["created_at"] = user.created_at;
    user_json["updated_at"] = user.updated_at;
    user_json["current_organization"] = user.current_organization;
    user_json["resource_type"] = user.resource_type;
    user_json["locale"] = user.locale;

    CacheEntry entry{user_json, now_iso8601(), USER_TTL_SECONDS};
    json entry_json = entry;
    write_cache_file("user.json", entry_json);
    print_verbose("Cache: user cached (TTL 24h)");
}

// --- Event types cache ---

std::optional<std::vector<EventType>> Cache::get_event_types() {
    if (is_cache_disabled()) {
        print_verbose("Cache: bypassed (--no-cache)");
        return std::nullopt;
    }

    auto file_data = read_cache_file("event_types.json");
    if (file_data.is_null() || !file_data.is_object()) {
        return std::nullopt;
    }

    CacheEntry entry = file_data.get<CacheEntry>();
    if (!is_entry_fresh(entry)) {
        print_verbose("Cache: event_types cache expired");
        return std::nullopt;
    }

    if (!entry.data.is_array()) {
        return std::nullopt;
    }

    print_verbose("Cache: event_types cache hit (" +
                  std::to_string(entry.data.size()) + " types)");
    return entry.data.get<std::vector<EventType>>();
}

void Cache::set_event_types(const std::vector<EventType>& types) {
    json types_json = json::array();
    for (const auto& et : types) {
        json j;
        j["uri"] = et.uri;
        j["name"] = et.name;
        j["active"] = et.active;
        if (et.booking_method.has_value()) j["booking_method"] = et.booking_method.value();
        j["slug"] = et.slug;
        j["scheduling_url"] = et.scheduling_url;
        j["duration"] = et.duration;
        j["kind"] = et.kind;
        if (et.pooling_type.has_value()) j["pooling_type"] = et.pooling_type.value();
        j["type"] = et.type;
        j["color"] = et.color;
        j["created_at"] = et.created_at;
        j["updated_at"] = et.updated_at;
        if (et.internal_note.has_value()) j["internal_note"] = et.internal_note.value();
        if (et.description_plain.has_value()) j["description_plain"] = et.description_plain.value();
        if (et.description_html.has_value()) j["description_html"] = et.description_html.value();
        // profile
        json profile;
        profile["type"] = et.profile.type;
        profile["name"] = et.profile.name;
        profile["owner"] = et.profile.owner;
        j["profile"] = profile;
        j["secret"] = et.secret;
        if (et.deleted_at.has_value()) j["deleted_at"] = et.deleted_at.value();
        j["admin_managed"] = et.admin_managed;
        // custom_questions
        json cqs = json::array();
        for (const auto& cq : et.custom_questions) {
            json qj;
            qj["name"] = cq.name;
            qj["type"] = cq.type;
            qj["position"] = cq.position;
            qj["enabled"] = cq.enabled;
            qj["required"] = cq.required;
            qj["answer_choices"] = cq.answer_choices;
            qj["include_other"] = cq.include_other;
            cqs.push_back(qj);
        }
        j["custom_questions"] = cqs;
        if (et.is_paid.has_value()) j["is_paid"] = et.is_paid.value();
        j["duration_options"] = et.duration_options;
        // locations
        json locs = json::array();
        for (const auto& loc : et.locations) {
            json lj;
            lj["kind"] = loc.kind;
            if (loc.location.has_value()) lj["location"] = loc.location.value();
            if (loc.additional_info.has_value()) lj["additional_info"] = loc.additional_info.value();
            if (loc.phone_number.has_value()) lj["phone_number"] = loc.phone_number.value();
            locs.push_back(lj);
        }
        j["locations"] = locs;
        types_json.push_back(j);
    }

    CacheEntry entry{types_json, now_iso8601(), EVENT_TYPES_TTL_SECONDS};
    json entry_json = entry;
    write_cache_file("event_types.json", entry_json);
    print_verbose("Cache: event_types cached (" +
                  std::to_string(types.size()) + " types, TTL 1h)");
}

// --- Invalidation ---

void Cache::clear_all() {
    std::error_code ec;
    std::filesystem::remove(cache_dir_ + "user.json", ec);
    std::filesystem::remove(cache_dir_ + "event_types.json", ec);
}

void Cache::clear_user() {
    std::error_code ec;
    std::filesystem::remove(cache_dir_ + "user.json", ec);
}

void Cache::clear_event_types() {
    std::error_code ec;
    std::filesystem::remove(cache_dir_ + "event_types.json", ec);
}

bool Cache::is_fresh(const std::string& key) const {
    std::string filename = key + ".json";
    auto file_data = read_cache_file(filename);
    if (file_data.is_null() || !file_data.is_object()) {
        return false;
    }
    CacheEntry entry = file_data.get<CacheEntry>();
    return is_entry_fresh(entry);
}

std::string Cache::cache_dir() const {
    return cache_dir_;
}

Cache::CacheStatus Cache::status() const {
    CacheStatus s;
    s.cache_path = cache_dir_;

    auto user_data = read_cache_file("user.json");
    if (!user_data.is_null() && user_data.is_object()) {
        CacheEntry entry = user_data.get<CacheEntry>();
        s.user_cached = is_entry_fresh(entry);
        s.user_cached_at = entry.cached_at;
    }

    auto et_data = read_cache_file("event_types.json");
    if (!et_data.is_null() && et_data.is_object()) {
        CacheEntry entry = et_data.get<CacheEntry>();
        s.event_types_cached = is_entry_fresh(entry);
        s.event_types_cached_at = entry.cached_at;
        if (entry.data.is_array()) {
            s.event_types_count = static_cast<int>(entry.data.size());
        }
    }

    return s;
}
