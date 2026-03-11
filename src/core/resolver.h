#pragma once
#include <string>
#include <vector>
#include "core/cache.h"
#include "modules/users/model.h"
#include "modules/event_types/model.h"

class Resolver {
public:
    explicit Resolver(Cache& cache);

    // Resolve a user-provided identifier to a full event type URI.
    // Strategy (Resolution E-04):
    //   1. Full URI -> pass through unchanged
    //   2. Exact UUID (36-char hyphenated) -> expand to full URI
    //   3. UUID prefix (8+ chars hex) -> match against cached; fail if ambiguous
    //   4. Name match (case-insensitive exact, then substring)
    //   5. Slug match (case-insensitive exact)
    //
    // Throws CalendlyError if ambiguous (lists matches) or not found.
    std::string resolve_event_type(const std::string& identifier);

    // Get current user URI (from cache, or fetch and cache)
    std::string get_user_uri();

    // Get current organization URI (from cache, or fetch and cache)
    std::string get_org_uri();

private:
    Cache& cache_;
    std::vector<EventType> ensure_event_types_cached();
    User ensure_user_cached();
};
