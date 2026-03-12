#include "core/resolver.h"
#include "core/error.h"
#include "core/filter.h"
#include "core/output.h"
#include "core/paginator.h"
#include "modules/users/api.h"
#include "modules/event_types/api.h"
#include <algorithm>
#include <cctype>

Resolver::Resolver(Cache& cache) : cache_(cache) {}

// --- Internal helpers ---

static std::string to_lower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

static bool is_full_uri(const std::string& s) {
    return s.find("https://api.calendly.com/") == 0;
}

static bool is_uuid_like(const std::string& s) {
    // 36-char hyphenated UUID: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
    if (s.size() != 36) return false;
    for (size_t i = 0; i < s.size(); ++i) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            if (s[i] != '-') return false;
        } else {
            if (!std::isxdigit(static_cast<unsigned char>(s[i]))) return false;
        }
    }
    return true;
}

static bool is_hex_prefix(const std::string& s) {
    if (s.size() < 8) return false;
    return std::all_of(s.begin(), s.end(), [](unsigned char c) {
        return std::isxdigit(c) || c == '-';
    });
}

User Resolver::ensure_user_cached() {
    auto cached = cache_.get_user();
    if (cached.has_value()) {
        return cached.value();
    }

    print_verbose("Resolver: fetching user profile...");
    auto user = users_api::get_me();
    cache_.set_user(user);
    return user;
}

std::vector<EventType> Resolver::ensure_event_types_cached() {
    auto cached = cache_.get_event_types();
    if (cached.has_value()) {
        return cached.value();
    }

    print_verbose("Resolver: fetching event types...");
    auto user = ensure_user_cached();

    // Fetch all event types using paginator
    event_types_api::ListOptions list_opts;
    list_opts.user_uri = user.uri;
    list_opts.count = 100;

    PaginationOptions page_opts;
    page_opts.count = 100;
    page_opts.fetch_all = true;

    CursorPaginator<EventType> paginator(
        [&list_opts](const std::optional<std::string>& token) {
            auto opts = list_opts;
            opts.page_token = token;
            return event_types_api::list(opts);
        },
        page_opts
    );

    auto all_types = paginator.fetch_all();
    cache_.set_event_types(all_types);
    return all_types;
}

std::string Resolver::get_user_uri() {
    return ensure_user_cached().uri;
}

std::string Resolver::get_org_uri() {
    return ensure_user_cached().current_organization;
}

std::string Resolver::resolve_event_type(const std::string& identifier) {
    // 1. Full URI passthrough
    if (is_full_uri(identifier)) {
        print_verbose("Resolver: full URI passthrough");
        return identifier;
    }

    // 2. Exact UUID -> expand
    if (is_uuid_like(identifier)) {
        print_verbose("Resolver: exact UUID -> expanding to URI");
        return build_uri("event_types", identifier);
    }

    // For prefix/name/slug matching, we need cached event types
    auto types = ensure_event_types_cached();

    // 3. UUID prefix match (8+ hex chars)
    if (is_hex_prefix(identifier)) {
        std::string lower_id = to_lower(identifier);
        std::vector<const EventType*> matches;

        for (const auto& et : types) {
            std::string uuid = to_lower(extract_uuid(et.uri));
            if (uuid.find(lower_id) == 0) {
                matches.push_back(&et);
            }
        }

        if (matches.size() == 1) {
            print_verbose("Resolver: UUID prefix match -> " + matches[0]->name);
            return matches[0]->uri;
        }
        if (matches.size() > 1) {
            std::string msg = "Ambiguous UUID prefix '" + identifier + "'. Matches:\n";
            for (const auto* m : matches) {
                msg += "  - " + m->name + " (" + extract_uuid(m->uri) + ")\n";
            }
            throw CalendlyError(ErrorKind::Validation, msg);
        }
        // Fall through to name/slug matching
    }

    // 4. Name match (case-insensitive exact)
    std::string lower_id = to_lower(identifier);
    for (const auto& et : types) {
        if (to_lower(et.name) == lower_id) {
            print_verbose("Resolver: exact name match -> " + et.name);
            return et.uri;
        }
    }

    // 4b. Slug match (case-insensitive exact) — checked before substring
    for (const auto& et : types) {
        if (to_lower(et.slug) == lower_id) {
            print_verbose("Resolver: slug match -> " + et.name);
            return et.uri;
        }
    }

    // 4c. Name substring match
    std::vector<const EventType*> name_matches;
    for (const auto& et : types) {
        if (to_lower(et.name).find(lower_id) != std::string::npos) {
            name_matches.push_back(&et);
        }
    }

    if (name_matches.size() == 1) {
        print_verbose("Resolver: name substring match -> " + name_matches[0]->name);
        return name_matches[0]->uri;
    }
    if (name_matches.size() > 1) {
        std::string msg = "Ambiguous event type name '" + identifier + "'. Matches:\n";
        for (const auto* m : name_matches) {
            msg += "  - " + m->name + " (" + extract_uuid(m->uri) + ")\n";
        }
        throw CalendlyError(ErrorKind::Validation, msg);
    }

    // Not found
    std::string msg = "No event type found matching '" + identifier + "'.";
    if (!types.empty()) {
        msg += "\nAvailable event types:\n";
        for (const auto& et : types) {
            msg += "  - " + et.name + " (slug: " + et.slug + ")\n";
        }
    }
    throw CalendlyError(ErrorKind::NotFound, msg);
}
