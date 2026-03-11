# Calendly CLI -- Architecture Document

> Single source of truth for all implementation work.
> Generated from the Calendly REST API specs (`docs/api_spec/`) and binding resolutions (`docs/scrutiny/RESOLUTIONS.md`).

---

## Table of Contents

1. [API Coverage Map](#1-api-coverage-map)
2. [Data Models](#2-data-models)
3. [Module Architecture](#3-module-architecture)
4. [Core Infrastructure](#4-core-infrastructure)
5. [CLI Command Tree](#5-cli-command-tree)
6. [Build System](#6-build-system)
7. [Implementation Order](#7-implementation-order)
8. [Testing Strategy](#8-testing-strategy)

---

## 1. API Coverage Map

Every Calendly REST API endpoint, categorized by module, CLI inclusion, and command mapping.

Legend:
- **Yes** -- Include in CLI v1.0
- **No** -- Skip (out of scope, webhooks, admin-only)
- **Later** -- Include in v1.1+

---

### 1.1 Users Module

| Endpoint | Include | CLI Command | Notes |
|----------|---------|-------------|-------|
| `GET /users/me` | Yes | `calendly me` | Current authenticated user. Cached for 24h. |
| `GET /users/{uuid}` | Later | -- | Deferred to v1.1 (Resolution A-03) |

---

### 1.2 Event Types Module

| Endpoint | Include | CLI Command | Notes |
|----------|---------|-------------|-------|
| `GET /event_types` | Yes | `calendly event-types list` | Paginated list of event types for a user |
| `GET /event_types/{uuid}` | Yes | `calendly event-types show <uuid>` | Full event type detail including locations, custom questions |
| `GET /event_type_available_times` | Yes | `calendly event-types available-times <uuid>` | Available time slots (max 7-day window, no pagination) |

---

### 1.3 Events Module

| Endpoint | Include | CLI Command | Notes |
|----------|---------|-------------|-------|
| `GET /scheduled_events/{uuid}` | Yes | `calendly events show <uuid>` | Single event detail. Response wrapped in `{"resource": {...}}` (Resolution B-02, deferred to QA). |
| `GET /scheduled_events` | Yes | `calendly events list` | **New endpoint** (Resolution A-01). Paginated list with date/status filters. |
| `POST /scheduled_events` | Yes | `calendly events create` (hidden alias) | Creates event + invitee. `resource` is a URI string, NOT an object (Resolution B-03). |
| `POST /scheduled_events/{uuid}/cancellation` | Yes | `calendly events cancel <uuid>` | **New endpoint** (Resolution A-02). Cancel a scheduled event. |

---

### 1.4 Invitees Module

| Endpoint | Include | CLI Command | Notes |
|----------|---------|-------------|-------|
| `POST /invitees` | Yes | `calendly invitees book` (hidden alias) | Book a meeting. Same request body as POST /scheduled_events. Flat response (Resolution B-01). |
| `GET /scheduled_events/{uuid}/invitees` | Yes | `calendly invitees list <event_uuid>` | Paginated invitee list for a scheduled event |
| `GET /invitees/{invitee_uuid}` | Yes | `calendly invitees show <invitee_uuid>` | Returns distinct InviteRecord type (Resolution B-04). Uses `/invites/` path prefix. |

---

### 1.5 Organizations Module

| Endpoint | Include | CLI Command | Notes |
|----------|---------|-------------|-------|
| `GET /organizations/{uuid}/invitations` | Yes | `calendly org invitations` | List org invitations. Org UUID auto-resolved from cached user. |
| `POST /organizations/{uuid}/invitations` | Later | -- | Deferred to v1.1 (Resolution A-04) |
| `DELETE /organizations/{uuid}/invitations/{uuid}` | Later | -- | Deferred to v1.1 (Resolution A-04) |
| `GET /organization_memberships` | Later | -- | Deferred to v1.1 (Resolution A-04) |
| `DELETE /organization_memberships/{uuid}` | Later | -- | Deferred to v1.1 (Resolution A-04) |

---

### 1.6 Activity Log Module

| Endpoint | Include | CLI Command | Notes |
|----------|---------|-------------|-------|
| `GET /activity_log_entries` | Yes | `calendly activity-log list` | Extended pagination response with `last_event_time`, `total_count`, `exceeds_max_total_count` (Resolution H-02). |

---

### 1.7 One-Off Event Types Module

| Endpoint | Include | CLI Command | Notes |
|----------|---------|-------------|-------|
| `POST /one_off_event_types` | Yes | `calendly one-off create` | Create single-use scheduling link. Response schema undocumented (Resolution G-08). |

---

### 1.8 Composite Commands

| Command | Include | CLI Command | Notes |
|---------|---------|-------------|-------|
| Book (chains event-types list, available-times, POST /scheduled_events) | Yes | `calendly book` | Primary user-facing booking command (Resolution E-01). Replaces `events create` and `invitees book` as the default. |

---

### 1.9 Excluded APIs

| Category | Reason |
|----------|--------|
| Webhooks (`POST /webhook_subscriptions`, etc.) | Server-side subscription feature, incompatible with CLI scope |
| No-show (`POST /invitees/{uuid}/no_show`, `DELETE`) | Deferred to v1.1 (Resolution A-05) |
| Scheduling Links, Routing Forms, GDPR | Rejected for v1.0 scope (Resolution A-06) |

---

## 2. Data Models

C++ structs for each core object type. All structs use `std::optional<T>` for nullable fields and `std::string` for URIs/dates (ISO 8601 strings). JSON parsing uses nlohmann/json ADL (`from_json`).

### 2.1 Common Types

```cpp
// src/core/types.h

#include <optional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

// --- Pagination ---

struct CursorPageInfo {
    int count = 0;
    std::optional<std::string> next_page;           // Full URL for next page
    std::optional<std::string> previous_page;       // Full URL for previous page
    std::optional<std::string> next_page_token;     // Token for page_token param
    std::optional<std::string> previous_page_token; // Token for previous page
};

void from_json(const json& j, CursorPageInfo& p);

template<typename T>
struct CursorPage {
    std::vector<T> collection;
    CursorPageInfo pagination;
};

template<typename T>
void from_json(const json& j, CursorPage<T>& page);

// --- Output Format ---

enum class OutputFormat {
    Table,
    Detail,
    Json,
    Csv
};

// --- Tracking Object ---

struct Tracking {
    std::optional<std::string> utm_campaign;
    std::optional<std::string> utm_source;
    std::optional<std::string> utm_medium;
    std::optional<std::string> utm_content;
    std::optional<std::string> utm_term;
    std::optional<std::string> salesforce_uuid;
};

void from_json(const json& j, Tracking& t);
json to_json_obj(const Tracking& t);

// --- Question And Answer ---

struct QuestionAndAnswer {
    std::string question;
    std::string answer;
    std::optional<int> position;  // Required in requests, optional in GET responses (Resolution D-05)
};

void from_json(const json& j, QuestionAndAnswer& qa);
json to_json_obj(const QuestionAndAnswer& qa);

// --- Cancellation Object ---
// Always present in invitee responses (never null/absent).
// May be empty object {} when no cancellation has occurred (Resolution H-03).

struct Cancellation {
    std::optional<std::string> canceled_by;  // Absent when cancellation is {} (Resolution D-04)
    std::optional<std::string> reason;
};

void from_json(const json& j, Cancellation& c);

// --- Error Response ---

struct CalendlyApiError {
    std::string title;
    std::string message;
};

void from_json(const json& j, CalendlyApiError& e);
```

**Pagination notes:**
- All paginated endpoints use `CursorPage<T>`.
- Activity log response extends `CursorPage<T>` with extra fields (see ActivityLogResponse).
- `next_page_token == std::nullopt` means last page.

---

### 2.2 Enum Definitions

Per RESOLUTIONS.md C-01, C-02, C-03:

```cpp
// src/core/types.h (continued)

// --- Event Status (Resolution C-02) ---
// Uses "cancelled" (British spelling, double 'l').
// Implementation accepts both "cancelled" and "canceled" defensively.

enum class EventStatus {
    Confirmed,
    Cancelled,
    Unknown
};

EventStatus parse_event_status(const std::string& s);
std::string to_string(EventStatus s);

// --- Invitee Status (Resolution C-01) ---
// Uses "canceled" (American spelling, single 'l').
// Superset across all endpoints.

enum class InviteeStatus {
    Active,
    Canceled,
    Accepted,   // GET /invitees/{uuid} only
    Declined,   // GET /invitees/{uuid} only
    Unknown     // Forward-compatibility fallback
};

InviteeStatus parse_invitee_status(const std::string& s);
std::string to_string(InviteeStatus s);

// --- Location Kind (Resolution C-03) ---
// Canonical superset with alias resolution.

enum class LocationKind {
    Physical,
    AskInvitee,
    Custom,
    ZoomConference,
    GoogleConference,
    MicrosoftTeamsConference,
    OutboundCall,
    InboundCall,
    GoToMeeting,
    Webex,
    Unknown
};

LocationKind parse_location_kind(const std::string& s);
std::string to_string(LocationKind k);

// Alias resolution:
//   "zoom"             -> ZoomConference
//   "google_hangouts"  -> GoogleConference
//   "microsoft_teams"  -> MicrosoftTeamsConference
//   "phone_call"       -> OutboundCall
//   "virtual"          -> Unknown (deferred to QA, G-10)
```

---

### 2.3 User

```cpp
// src/modules/users/model.h

struct User {
    std::string uri;                              // https://api.calendly.com/users/AAAA...
    std::string name;
    std::string slug;
    std::string email;
    std::string scheduling_url;
    std::string timezone;                         // IANA timezone (e.g. "America/New_York")
    std::string time_notation;                    // "12h" or "24h"
    std::optional<std::string> avatar_url;        // Nullable
    std::string created_at;
    std::string updated_at;
    std::string current_organization;             // Full org URI
    std::string resource_type;                    // Always "User"
    std::string locale;                           // e.g. "en"
};

void from_json(const json& j, User& u);
```

**Parsed from:** `GET /users/me` response -> `resource` object.

**Table columns:** `name`, `email`, `timezone`, `scheduling_url`

**Detail view:** All fields.

---

### 2.4 EventType

```cpp
// src/modules/event_types/model.h

struct EventTypeProfile {
    std::string type;                             // "User" or "Team"
    std::string name;
    std::string owner;                            // User or org URI
};

void from_json(const json& j, EventTypeProfile& p);

struct Location {
    std::string kind;                             // See LocationKind enum
    std::optional<std::string> location;          // URL or address; null for "ask_invitee"
    std::optional<std::string> additional_info;
    std::optional<std::string> phone_number;
};

void from_json(const json& j, Location& l);

struct CustomQuestion {
    std::string name;                             // Question text
    std::string type;                             // "text", "dropdown", "radio", "checkbox", "phone_number"
    int position = 0;
    bool enabled = true;
    bool required = false;
    std::vector<std::string> answer_choices;
    bool include_other = false;
};

void from_json(const json& j, CustomQuestion& q);

struct EventType {
    std::string uri;
    std::string name;
    bool active = true;
    std::optional<std::string> booking_method;    // "poll" or null
    std::string slug;
    std::string scheduling_url;
    int duration = 0;                             // Minutes
    std::string kind;                             // "solo" or "group"
    std::optional<std::string> pooling_type;      // "round_robin", "collective", or null
    std::string type;                             // Always "EventType"
    std::string color;                            // Hex (e.g. "#fff200")
    std::string created_at;
    std::string updated_at;
    std::optional<std::string> internal_note;
    std::optional<std::string> description_plain; // Nullable (Resolution D-03)
    std::optional<std::string> description_html;  // Nullable (Resolution D-03)
    EventTypeProfile profile;
    bool secret = false;
    std::optional<std::string> deleted_at;
    bool admin_managed = false;
    std::vector<CustomQuestion> custom_questions;

    // Detail-only fields (not in list response)
    std::optional<bool> is_paid;                  // Only in GET /event_types/{uuid} (Resolution G-11)
    std::vector<int> duration_options;             // e.g. [15, 30, 60]
    std::vector<Location> locations;
};

void from_json(const json& j, EventType& et);
```

**Table columns (list):** `name`, `duration`, `kind`, `active`, `slug`, `scheduling_url`

**Detail view:** All fields including `locations`, `custom_questions`, `description_plain`.

---

### 2.5 AvailableTime

```cpp
// src/modules/event_types/model.h (continued)

struct AvailableTime {
    std::string status;                           // "available"
    std::string start_time;                       // ISO 8601
    int invitees_remaining = 1;                   // Spots left (1 for solo, N for group)
};

void from_json(const json& j, AvailableTime& at);
```

**Table columns:** `start_time` (formatted in user tz), `invitees_remaining`

---

### 2.6 ScheduledEvent

```cpp
// src/modules/events/model.h

struct InviteesCounter {
    int total = 0;
    int confirmed = 0;
    int declined = 0;
};

void from_json(const json& j, InviteesCounter& ic);

struct ScheduledEvent {
    std::string uri;
    std::optional<std::string> uuid;              // Present in GET response, extract from uri if absent
    std::string created_at;
    std::string start_time;
    std::string end_time;
    std::string status;                           // "confirmed" or "cancelled" (British, double 'l') (Resolution C-02)
    std::string event_type;                       // Event type URI
    std::optional<std::string> location;          // URL or address or null (Resolution G-09)
    InviteesCounter invitees_counter;
    std::vector<QuestionAndAnswer> questions_and_answers;
};

void from_json(const json& j, ScheduledEvent& se);
```

**Parsed from:**
- `GET /scheduled_events/{uuid}` -> wrapped in `{"resource": {...}}` (Resolution B-02, deferred QA).
- `GET /scheduled_events` -> `collection[]` items.

**Table columns (list):** `uuid` (first 8 chars), `start_time`, `end_time`, `status`, `location`

**Detail view:** All fields including `invitees_counter`, `questions_and_answers`.

---

### 2.7 CreateEventRequest / CreateEventResponse

```cpp
// src/modules/events/model.h (continued)

struct InviteeRequest {
    std::string name;
    std::optional<std::string> first_name;        // Optional (Resolution D-01)
    std::optional<std::string> last_name;         // Optional (Resolution D-01)
    std::string email;
    std::string timezone;                         // IANA tz
    std::optional<std::string> text_reminder_number;
};

json to_json_obj(const InviteeRequest& ir);

struct LocationRequest {
    std::string kind;                             // See LocationKind enum
    std::optional<std::string> location;          // Required for "physical"
};

json to_json_obj(const LocationRequest& lr);

struct CreateEventRequest {
    std::string event_type;                       // Full event type URI
    std::string start_time;                       // ISO 8601
    InviteeRequest invitee;
    std::optional<LocationRequest> location;
    std::vector<QuestionAndAnswer> questions_and_answers;
    std::optional<Tracking> tracking;
    std::vector<std::string> event_guests;        // Email addresses
};

json to_json_obj(const CreateEventRequest& req);

// Response from POST /scheduled_events
// IMPORTANT: `resource` is a URI string, NOT an object (Resolution B-03)
struct CreateEventResponse {
    std::string resource;                         // URI string to the created event
    Invitee invitee;                              // Full invitee object (see 2.8)
};

void from_json(const json& j, CreateEventResponse& r);
```

---

### 2.8 Invitee

```cpp
// src/modules/invitees/model.h

// Full invitee object returned by POST /invitees, POST /scheduled_events,
// and GET /scheduled_events/{uuid}/invitees (list).

struct Invitee {
    std::string uri;
    std::string email;
    std::optional<std::string> first_name;
    std::optional<std::string> last_name;
    std::string name;
    std::string status;                           // "active" or "canceled" (American single 'l')
    std::vector<QuestionAndAnswer> questions_and_answers;
    std::optional<std::string> timezone;
    std::string event;                            // Scheduled event URI
    std::string created_at;
    std::string updated_at;
    Tracking tracking;                            // Always present, may be {} (Resolution H-03)
    std::optional<std::string> text_reminder_number;
    bool rescheduled = false;
    std::optional<std::string> old_invitee;       // URI of previous invitee if rescheduled
    std::optional<std::string> new_invitee;       // URI of new invitee if rescheduled
    std::string cancel_url;
    std::string reschedule_url;
    std::optional<std::string> routing_form_submission;
    Cancellation cancellation;                    // Always present, may be {} (Resolution H-03)
    std::optional<json> payment;                  // Opaque, null if not paid
    std::optional<json> no_show;                  // Opaque, null if no no-show
    std::optional<json> reconfirmation;           // Opaque, null if not configured
    std::optional<std::string> scheduling_method; // e.g. "instant_book"
    std::optional<std::string> invitee_scheduled_by; // URI of user who scheduled
};

void from_json(const json& j, Invitee& inv);
```

**Table columns (list):** `name`, `email`, `status`, `timezone`, `created_at`

**Detail view:** All fields.

---

### 2.9 InviteRecord

```cpp
// src/modules/invitees/model.h (continued)

// Distinct resource type returned by GET /invitees/{invitee_uuid} (Resolution B-04).
// Uses /invites/ path prefix. Slim record with event timing.

struct InviteRecord {
    std::string uri;                              // https://api.calendly.com/invites/BBBB...
    std::string invitee_uuid;
    std::string status;                           // Full superset: active, canceled, accepted, declined
    std::string created_at;
    std::string start_time;
    std::string end_time;
    std::string event;                            // Scheduled event URI
    std::string invitation_secret;                // Sensitive token
};

void from_json(const json& j, InviteRecord& ir);
```

**Detail view:** All fields except `invitation_secret` (shown only with `--json`).

---

### 2.10 BookingRequest / BookingResponse

```cpp
// src/modules/invitees/model.h (continued)

// Request body for POST /invitees is identical to CreateEventRequest (Resolution: events.md sec 6).
using BookingRequest = CreateEventRequest;

// Response from POST /invitees (Resolution B-01)
// Working assumption: flat response (no resource/invitee wrapper).
// Deferred to QA (G-03, G-04).
struct BookingResponse {
    std::string event;                            // Scheduled event URI
    std::string status;
    std::optional<std::string> timezone;
    std::string cancel_url;
    std::string reschedule_url;
    std::vector<QuestionAndAnswer> questions_and_answers;
    std::optional<std::string> text_reminder_number;
    std::string created_at;
    std::string updated_at;
    bool rescheduled = false;
    std::optional<std::string> old_invitee;
    std::optional<std::string> new_invitee;
    std::string uri;
    std::string email;
    std::string name;
    std::optional<std::string> first_name;
    std::optional<std::string> last_name;
};

void from_json(const json& j, BookingResponse& r);
```

**Note:** If QA reveals the response is actually wrapped in `{"resource": ..., "invitee": {...}}`, the implementation must handle both shapes defensively.

---

### 2.11 CancellationRequest / CancellationResponse

```cpp
// src/modules/events/model.h (continued)

// POST /scheduled_events/{uuid}/cancellation (Resolution A-02)

struct CancellationRequest {
    std::optional<std::string> reason;
};

json to_json_obj(const CancellationRequest& req);

struct CancellationResponse {
    std::string canceled_by;
    std::optional<std::string> reason;
};

void from_json(const json& j, CancellationResponse& r);
```

---

### 2.12 OrganizationInvitation

```cpp
// src/modules/organizations/model.h

struct OrganizationInvitation {
    std::string uri;
    std::string organization;                     // Org URI
    std::string email;
    std::string status;                           // "pending", "accepted", "declined"
    std::string created_at;
    std::string updated_at;
    std::optional<std::string> last_sent_at;
    std::optional<std::string> user;              // User URI, only when status == "accepted"
};

void from_json(const json& j, OrganizationInvitation& oi);
```

**Table columns:** `email`, `status`, `created_at`, `last_sent_at`

---

### 2.13 ActivityLogEntry / ActivityLogActor

```cpp
// src/modules/activity_log/model.h

struct ActorOrganization {
    std::string uri;
    std::string role;                             // "Owner", "Admin", "User"
};

void from_json(const json& j, ActorOrganization& ao);

struct ActorGroup {
    std::string uri;
    std::string name;
    std::string role;                             // "Admin", "Member"
};

void from_json(const json& j, ActorGroup& ag);

struct ActivityLogActor {
    std::string uri;
    std::string type;                             // Typically "User"
    std::optional<ActorOrganization> organization; // Nullable (Resolution H-07)
    std::optional<ActorGroup> group;               // Nullable (Resolution H-07)
    std::string display_name;
    std::string alternative_identifier;           // Typically email
};

void from_json(const json& j, ActivityLogActor& a);

struct ActivityLogEntry {
    std::string occurred_at;
    std::string uri;
    std::string namespace_val;                    // "User", "EventType", etc.
    std::string action;                           // "Add", "Remove", etc.
    ActivityLogActor actor;
    std::string fully_qualified_name;             // "Namespace.Action"
    json details;                                 // Opaque, dynamic structure
    std::string organization;                     // Org URI
};

void from_json(const json& j, ActivityLogEntry& e);

// Extended response for activity log (Resolution H-02).
// NOT a generic CursorPage<T> -- includes extra top-level fields.
struct ActivityLogResponse {
    std::vector<ActivityLogEntry> collection;
    CursorPageInfo pagination;
    std::optional<std::string> last_event_time;   // Newest entry timestamp, null if empty
    int total_count = 0;
    bool exceeds_max_total_count = false;
};

void from_json(const json& j, ActivityLogResponse& r);
```

**Table columns:** `occurred_at`, `fully_qualified_name`, `actor.display_name`, `details` (summary)

---

### 2.14 OneOffEventTypeRequest

```cpp
// src/modules/one_off/model.h

struct DateSetting {
    std::string type;                             // "date_range"
    std::string start_date;                       // "YYYY-MM-DD"
    std::string end_date;                         // "YYYY-MM-DD"
};

json to_json_obj(const DateSetting& ds);

struct OneOffLocation {
    std::string kind;                             // See LocationKind enum
    std::optional<std::string> location;
    std::optional<std::string> additional_info;   // Sent as "additonal_info" (API typo, Resolution H-09, G-07)
};

json to_json_obj(const OneOffLocation& loc);

struct OneOffEventTypeRequest {
    std::string name;
    std::string host;                             // User URI
    std::vector<std::string> co_hosts;            // User URIs
    int duration = 30;                            // Minutes
    std::string timezone;                         // IANA tz
    DateSetting date_setting;
    OneOffLocation location;
};

json to_json_obj(const OneOffEventTypeRequest& req);

// Response is undocumented (Resolution G-08).
// All fields treated as optional until verified against live API.
struct OneOffEventTypeResponse {
    std::optional<std::string> uri;
    std::optional<std::string> name;
    std::optional<std::string> scheduling_url;
    std::optional<int> duration;
    std::optional<std::string> kind;
    std::optional<std::string> slug;
    std::optional<bool> secret;
    std::optional<std::string> created_at;
    std::optional<std::string> updated_at;
    json raw;                                     // Store raw JSON alongside typed fields
};

void from_json(const json& j, OneOffEventTypeResponse& r);
```

---

## 3. Module Architecture

### 3.1 Directory Structure

```
src/
  main.cpp                                — Entry point, CLI11 app setup, command registration
  core/
    types.h                               — CursorPageInfo, CursorPage<T>, Tracking, Q&A, Cancellation, enums
    http_client.h / http_client.cpp       — libcurl RAII wrapper, rate limit header extraction
    rest_client.h / rest_client.cpp       — URL building, auth headers, JSON parsing, GET/POST/DELETE
    auth.h / auth.cpp                     — CALENDLY_API_TOKEN env > config file, Bearer token
    config.h / config.cpp                 — TOML config (~/.config/calendly/config.toml)
    output.h / output.cpp                 — TableRenderer, DetailRenderer, JSON/CSV output
    color.h / color.cpp                   — ANSI colors, status-aware coloring, pipe detection
    error.h / error.cpp                   — CalendlyError, check_http_status, with_retry, format_error
    paginator.h                           — CursorPaginator<T> template (page_token based auto-pagination)
    cache.h / cache.cpp                   — JSON file cache (users, event_types), TTL enforcement
    filter.h / filter.cpp                 — extract_uuid, build_uri, timezone validation, query param builders
    resolver.h / resolver.cpp             — Name-to-URI resolution (event type name -> URI, UUID expansion)
    version.h.in                          — Version template (configured by CMake)
  modules/
    users/
      model.h                             — User struct
      api.h / api.cpp                     — GET /users/me
      commands.h / commands.cpp           — `calendly me`
    event_types/
      model.h                             — EventType, EventTypeProfile, Location, CustomQuestion, AvailableTime
      api.h / api.cpp                     — GET /event_types, GET /event_types/{uuid}, GET /event_type_available_times
      commands.h / commands.cpp           — `event-types list`, `event-types show`, `event-types available-times`
    events/
      model.h                             — ScheduledEvent, InviteesCounter, CreateEventRequest/Response,
                                            CancellationRequest/Response
      api.h / api.cpp                     — GET /scheduled_events, GET /scheduled_events/{uuid},
                                            POST /scheduled_events, POST /scheduled_events/{uuid}/cancellation
      commands.h / commands.cpp           — `events list`, `events show`, `events create` (hidden), `events cancel`
    invitees/
      model.h                             — Invitee, InviteRecord, BookingRequest, BookingResponse
      api.h / api.cpp                     — POST /invitees, GET /scheduled_events/{uuid}/invitees,
                                            GET /invitees/{invitee_uuid}
      commands.h / commands.cpp           — `invitees book` (hidden), `invitees list`, `invitees show`
    organizations/
      model.h                             — OrganizationInvitation
      api.h / api.cpp                     — GET /organizations/{uuid}/invitations
      commands.h / commands.cpp           — `org invitations`
    activity_log/
      model.h                             — ActivityLogEntry, ActivityLogActor, ActorOrganization, ActorGroup,
                                            ActivityLogResponse
      api.h / api.cpp                     — GET /activity_log_entries
      commands.h / commands.cpp           — `activity-log list`
    one_off/
      model.h                             — OneOffEventTypeRequest, OneOffEventTypeResponse, DateSetting,
                                            OneOffLocation
      api.h / api.cpp                     — POST /one_off_event_types
      commands.h / commands.cpp           — `one-off create`
    book/
      commands.h / commands.cpp           — `calendly book` (composite command, chains event_types + events APIs)
    config/
      commands.h / commands.cpp           — `calendly config show`, `calendly config set`, `calendly config path`
    cache/
      commands.h / commands.cpp           — `calendly cache clear`, `calendly cache show`
```

### 3.2 File Count Summary

| Category | Files |
|----------|-------|
| Core infrastructure | 13 headers + 10 source = 23 files |
| Module headers (model + api + commands) | 10 modules x 3 headers = 30 files |
| Module sources (api + commands) | 10 modules x 2 source = 20 files |
| Book composite module | 2 files (commands.h + commands.cpp) |
| Config/Cache modules | 4 files (2 each, commands only) |
| main.cpp + version.h.in | 2 files |
| **Total** | **~81 files** |

### 3.3 Module Pattern

Each module follows the same pattern:

**model.h** -- Data structs + JSON parsing (`from_json`) + serialization (`to_json_obj`)
```cpp
// Pure data, no dependencies on other modules
struct MyEntity { ... };
void from_json(const json& j, MyEntity& e);
json to_json_obj(const MyEntity& e);  // Only for request types
```

**api.h / api.cpp** -- REST calls + response parsing
```cpp
// Depends on: core/rest_client, core/error, model.h
namespace my_module_api {
    CursorPage<MyEntity> list(const ListOptions& opts);
    MyEntity get(const std::string& uuid);
    MyResponse create(const MyRequest& req);
}
```

**commands.h / commands.cpp** -- CLI11 subcommand registration + handler functions
```cpp
// Depends on: api.h, core/output, core/filter, core/resolver
namespace my_module_commands {
    void register_commands(CLI::App& app);
}
// Each handler: parse CLI args -> validate -> call api -> format output
```

---

## 4. Core Infrastructure

### 4.1 HTTP Client (`src/core/http_client.h` / `http_client.cpp`)

libcurl RAII wrapper. Non-copyable, owns CURL handle.

```cpp
struct RateLimitInfo {
    int limit = 0;
    int remaining = 0;
    int reset_seconds = 0;
};

struct HttpResponse {
    long status_code;
    std::string body;
    std::string error_message;
    RateLimitInfo rate_limit;                      // Extracted from X-RateLimit-* headers
};

class HttpClient {
public:
    HttpClient();
    ~HttpClient();

    HttpClient(const HttpClient&) = delete;
    HttpClient& operator=(const HttpClient&) = delete;

    HttpResponse get(const std::string& url, const std::vector<std::string>& headers);
    HttpResponse post(const std::string& url, const std::string& body, const std::vector<std::string>& headers);

private:
    CURL* handle_;
    static size_t write_callback(char* ptr, size_t size, size_t nmemb, void* userdata);
    static size_t header_callback(char* buffer, size_t size, size_t nitems, void* userdata);
    RateLimitInfo parse_rate_limit_headers(const std::string& raw_headers);
};
```

**Key behaviors:**
- RAII: `curl_easy_init()` in constructor, `curl_easy_cleanup()` in destructor.
- Extracts `X-RateLimit-Limit`, `X-RateLimit-Remaining`, `X-RateLimit-Reset` from every response.
- Sets `User-Agent: calendly-cli/{version}`.
- Handles HTTPS via system CA bundle.

---

### 4.2 REST Client (`src/core/rest_client.h` / `rest_client.cpp`)

Higher-level client built on HttpClient. Handles URL construction, auth, JSON parsing, retries.

```cpp
class RestClient {
public:
    RestClient();

    // GET with automatic auth header injection
    json get(const std::string& path, const std::map<std::string, std::string>& params = {});

    // POST with JSON body
    json post(const std::string& path, const json& body);

    // Generic request with retry support
    json request_with_retry(
        const std::string& method,
        const std::string& path,
        const json& body = {},
        int max_retries = 3
    );

    // Access rate limit info from last request
    RateLimitInfo last_rate_limit() const;

private:
    HttpClient http_;
    std::string base_url_;                        // "https://api.calendly.com"
    std::string build_url(const std::string& path, const std::map<std::string, std::string>& params) const;
    std::vector<std::string> build_headers() const;
    void check_response(const HttpResponse& resp) const;
};
```

**Key behaviors:**
- `base_url_` is always `https://api.calendly.com`.
- `build_headers()` injects `Authorization: Bearer <token>` and `Content-Type: application/json`.
- `check_response()` throws `CalendlyError` for 4xx/5xx, parsing `title` and `message` from JSON body.
- `request_with_retry()` implements exponential backoff for 429 responses using `X-RateLimit-Reset`.

---

### 4.3 Auth (`src/core/auth.h` / `auth.cpp`)

Token resolution with precedence chain.

```cpp
// Resolution order:
// 1. CALENDLY_API_TOKEN environment variable
// 2. token field in ~/.config/calendly/config.toml
// 3. Error with instructions

std::string get_api_token();
bool has_api_token();
```

**Config file format** (`~/.config/calendly/config.toml`):
```toml
token = "your_personal_access_token_here"
```

---

### 4.4 Config (`src/core/config.h` / `config.cpp`)

TOML configuration management following XDG Base Directory Specification.

```cpp
struct CalendlyConfig {
    std::optional<std::string> token;
    std::optional<std::string> default_timezone;  // Override for display
    bool color_enabled = true;
    int default_count = 20;                       // Default page size
};

CalendlyConfig load_config();
void save_config(const CalendlyConfig& cfg);
std::string config_file_path();                   // ~/.config/calendly/config.toml
```

---

### 4.5 Output (`src/core/output.h` / `output.cpp`)

Formatted output for table, detail, JSON, and CSV modes.

```cpp
class TableRenderer {
public:
    void add_column(const std::string& header, int min_width = 0);
    void add_row(const std::vector<std::string>& values);
    void render(std::ostream& out) const;
};

class DetailRenderer {
public:
    void add_field(const std::string& label, const std::string& value);
    void add_field(const std::string& label, const std::optional<std::string>& value);
    void add_section(const std::string& title);
    void render(std::ostream& out) const;
};

void output_json(const json& j, std::ostream& out = std::cout);
void output_csv(const std::vector<std::string>& headers,
                const std::vector<std::vector<std::string>>& rows,
                std::ostream& out = std::cout);

OutputFormat get_output_format();
void set_output_format(OutputFormat fmt);
```

**Table formatting rules** (Resolution F-04):
- Shortened UUIDs: first 8 characters in table mode.
- Strip `https://api.calendly.com/` from URIs in table mode.
- Full URIs and UUIDs in `--json` mode.
- Relative time display in table mode ("in 2 hours", "tomorrow 3:00 PM").
- `--iso` flag for raw ISO 8601 timestamps.

---

### 4.6 Color (`src/core/color.h` / `color.cpp`)

ANSI color support with pipe detection and status-aware coloring.

```cpp
namespace color {
    void set_enabled(bool enabled);
    bool is_enabled();

    std::string bold(const std::string& text);
    std::string dim(const std::string& text);
    std::string green(const std::string& text);
    std::string red(const std::string& text);
    std::string yellow(const std::string& text);
    std::string cyan(const std::string& text);
    std::string blue(const std::string& text);
    std::string magenta(const std::string& text);

    // Status-aware coloring
    std::string event_status(const std::string& status);   // confirmed=green, cancelled=red
    std::string invitee_status(const std::string& status); // active=green, canceled=red, accepted=green, declined=yellow
    std::string org_invitation_status(const std::string& status);
}
```

**Auto-detection:** Disable color when stdout is not a TTY (piped), or when `NO_COLOR` env is set, or `--no-color` flag.

---

### 4.7 Error (`src/core/error.h` / `error.cpp`)

Error handling with structured error types and retry logic.

```cpp
class CalendlyError : public std::runtime_error {
public:
    CalendlyError(int status_code, const std::string& title, const std::string& message);

    int status_code() const;
    const std::string& title() const;
    const std::string& api_message() const;
};

// Throw CalendlyError if status code is 4xx/5xx
void check_http_status(const HttpResponse& resp);

// Retry with exponential backoff for 429 responses
template<typename Fn>
auto with_retry(Fn&& fn, int max_retries = 3) -> decltype(fn());

// Format error for display
std::string format_error(const CalendlyError& e);
```

**Error response parsing** (Resolution H-04):
```json
{
  "title": "string (error category)",
  "message": "string (human-readable description)"
}
```
Use permissive deserialization that ignores unknown fields.

---

### 4.8 Paginator (`src/core/paginator.h`)

Template for automatic cursor-based pagination (used by `--all` flag).

```cpp
template<typename T>
class CursorPaginator {
public:
    using FetchFn = std::function<CursorPage<T>(const std::optional<std::string>& page_token)>;

    explicit CursorPaginator(FetchFn fetch_fn);

    // Fetch a single page
    CursorPage<T> fetch_page(const std::optional<std::string>& page_token = std::nullopt);

    // Fetch ALL pages, concatenating collections
    std::vector<T> fetch_all();
};
```

---

### 4.9 Cache (`src/core/cache.h` / `cache.cpp`)

JSON file cache following XDG Base Directory Specification (Resolution F-01).

```cpp
// Cache location: $XDG_CACHE_HOME/calendly/ or ~/.cache/calendly/
// Files: user.json, event_types.json

struct CacheEntry {
    json data;
    std::string cached_at;                        // ISO 8601 timestamp
    int ttl_seconds = 0;
};

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

    // Cache file path
    std::string cache_dir() const;

private:
    std::string cache_dir_;
    json read_cache_file(const std::string& filename) const;
    void write_cache_file(const std::string& filename, const json& data) const;
};
```

**Cache table** (Resolution F-01):

| Entity | Cache | TTL | Invalidation |
|--------|-------|-----|-------------|
| User profile (`/users/me`) | YES | 24 hours | `calendly me --refresh`, token change |
| Event types list | YES | 1 hour | `calendly event-types list --refresh` |
| Available times | **NEVER** | N/A | Always volatile |
| Scheduled events | **NEVER** | N/A | Change frequently |
| Invitees | **NEVER** | N/A | Change frequently |
| Activity log | **NEVER** | N/A | Append-only |

---

### 4.10 Filter (`src/core/filter.h` / `filter.cpp`)

Utility functions for URI manipulation, UUID extraction, and query parameter construction.

```cpp
// Extract UUID from full Calendly URI
// "https://api.calendly.com/users/AAAA..." -> "AAAA..."
std::string extract_uuid(const std::string& uri);

// Build full URI from resource type and UUID
// ("users", "AAAA...") -> "https://api.calendly.com/users/AAAA..."
std::string build_uri(const std::string& resource_type, const std::string& uuid);

// Validate IANA timezone string
bool is_valid_timezone(const std::string& tz);

// Build query parameter string from map, URL-encoding values
std::string build_query_string(const std::map<std::string, std::string>& params);

// Parse ISO 8601 timestamp for display
std::string format_relative_time(const std::string& iso8601);
std::string format_local_time(const std::string& iso8601, const std::string& timezone);

// Validate count parameter (Resolution H-06)
int validate_count(int count);  // Enforces 1 <= count <= 100

// Sort parameter validation (Resolution C-04)
// Ensures "field:direction" format
std::string validate_sort(const std::string& sort);
```

---

### 4.11 Resolver (`src/core/resolver.h` / `resolver.cpp`)

Name-to-URI resolution for event types and other resources (Resolution E-04).

```cpp
class Resolver {
public:
    explicit Resolver(Cache& cache);

    // Resolve a user-provided identifier to a full event type URI.
    // Strategy (Resolution E-04):
    //   1. Exact UUID (36-char or 16-char) -> expand to full URI
    //   2. UUID prefix (8+ chars) -> match against cached resources; fail if ambiguous
    //   3. Name match (case-insensitive exact, then substring, then fuzzy)
    //   4. Slug match
    //   5. Full URI -> pass through unchanged
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
```

---

## 5. CLI Command Tree

### Global Flags

Applied to all commands:

| Flag | Type | Default | Description |
|------|------|---------|-------------|
| `--json` | flag | false | Output raw JSON response |
| `--csv` | flag | false | Output as CSV |
| `--no-color` | flag | false | Disable ANSI colors |
| `--no-cache` | flag | false | Bypass all cached data |
| `--verbose` | flag | false | Show debug info (request URLs, timing) |
| `--version` | flag | -- | Show version and exit |
| `--help` | flag | -- | Show help and exit |

---

### Group: Scheduling

#### `calendly book`

**The primary user-facing booking command** (Resolution E-01). Chains event-types list, available-times, and POST /scheduled_events.

```
calendly book \
  --type <name|uuid|slug>             # Required. Fuzzy-matched against event type names.
  --name <full_name>                  # Required. Invitee full name.
  --email <email>                     # Required. Invitee email.
  [--start <iso8601>]                 # Optional. Explicit start time.
  [--date <YYYY-MM-DD>]              # Optional. Picks first available slot on date.
  [--timezone <IANA>]                 # Optional. Defaults to cached user timezone.
  [--first-name <name>]              # Optional. Derived from --name if omitted.
  [--last-name <name>]               # Optional. Derived from --name if omitted.
  [--location-kind <kind>]           # Optional. See LocationKind enum.
  [--location <value>]               # Optional. Required for "physical" kind.
  [--guest <email>,...]              # Optional. Comma-separated guest emails.
  [--qa <question>::<answer>]        # Optional. Repeatable. Uses :: delimiter (Resolution E-07).
  [--first]                           # Optional. Non-interactive: pick first available slot.
  [--dry-run]                         # Optional. Preview without booking.
  [--yes / -y]                        # Optional. Skip confirmation prompt.
  [--json]                            # Optional. Output raw JSON.
```

**Internal flow:**
1. Auto-fetch user URI from cache (fetch if missing).
2. Resolve `--type` to event type URI via name/slug/UUID matching (Resolution E-04).
3. If `--start` not given: fetch available times for `--date` (or today+7d if neither) and either pick first (`--first`) or prompt interactively.
4. POST to `/scheduled_events`.
5. Display booking confirmation with event URI, start time, cancel URL.

**Output example (table mode):**
```
Booked! 30 Minute Meeting
  When:    Tomorrow, Mar 12 at 2:00 PM EST
  Who:     Jane Smith <jane@example.com>
  Event:   abc12345
  Cancel:  https://calendly.com/cancellations/AAAA...
```

---

#### `calendly events list`

List scheduled events with smart defaults (Resolution E-02).

```
calendly events list [--status <active|canceled>] [--min-start <iso8601>] [--max-start <iso8601>]
  [--count N] [--all] [--sort <field:dir>] [--invitee-email <email>]
  [--today] [--tomorrow] [--this-week] [--json]
```

| Flag | Type | Default | Description |
|------|------|---------|-------------|
| `--status` | string | -- | Filter by event status |
| `--min-start` | string | `now` | ISO 8601 lower bound on start time |
| `--max-start` | string | `now + 7 days` | ISO 8601 upper bound on start time |
| `--count` | integer | 20 | Results per page (1-100) |
| `--all` | flag | false | Auto-paginate all results |
| `--sort` | string | `start_time:asc` | Sort order (`field:direction` format) |
| `--invitee-email` | string | -- | Filter by invitee email |
| `--today` | flag | false | Shorthand: start/end of today |
| `--tomorrow` | flag | false | Shorthand: start/end of tomorrow |
| `--this-week` | flag | false | Shorthand: now through end of week |

**Output example (table mode):**
```
UUID      START TIME           END TIME             STATUS     LOCATION
abc12345  Tomorrow 2:00 PM     Tomorrow 2:30 PM     confirmed  https://meet.google.com/xyz
def67890  Mar 14 10:00 AM      Mar 14 11:00 AM      confirmed  123 Main St
```

---

#### `calendly events show <uuid>`

Show full details of a scheduled event.

```
calendly events show <uuid> [--json]
```

**Output example (detail mode):**
```
Event: abc12345
  Status:     confirmed
  Start:      Mar 12, 2026 2:00 PM EST
  End:        Mar 12, 2026 2:30 PM EST
  Type:       30 Minute Meeting
  Location:   https://meet.google.com/xyz-abc-def
  Invitees:   2 total, 1 confirmed, 1 declined
  Q&A:
    Name: John Doe
```

---

#### `calendly events create` (hidden alias)

```
calendly events create --event-type <uri> --start-time <iso8601> --name <name> --email <email>
  --timezone <tz> [--location-kind <kind>] [--location <value>] [--guest <email>,...]
  [--qa <q>::<a>] [--first-name <name>] [--last-name <name>] [--dry-run] [--yes] [--json]
```

Hidden from help. Scripts can use it directly. Users should prefer `calendly book`.

---

#### `calendly events cancel <uuid>`

Cancel a scheduled event (Resolution E-03).

```
calendly events cancel <uuid> [--reason <text>] [--yes / -y] [--json]
```

Requires confirmation prompt unless `--yes` is passed or stdin is not a TTY (Resolution F-05).

**Output example:**
```
Event abc12345 cancelled.
  Reason: Client requested reschedule
```

---

#### `calendly event-types list`

```
calendly event-types list [--active] [--inactive] [--sort <field:dir>] [--count N] [--all]
  [--page-token <token>] [--user <uri>] [--json]
```

| Flag | Type | Default | Description |
|------|------|---------|-------------|
| `--active` | flag | -- | Only active event types |
| `--inactive` | flag | -- | Only inactive event types |
| `--sort` | string | -- | Sort order (e.g. `name:asc`) |
| `--count` | integer | 20 | Results per page (1-100) |
| `--all` | flag | false | Auto-paginate all results |
| `--user` | string | current user | User URI |
| `--refresh` | flag | false | Force cache refresh |

**Output example (table mode):**
```
NAME                DURATION  KIND  ACTIVE  SLUG        URL
15 Minute Meeting   15        solo  true    15min       https://calendly.com/acme/15min
30 Minute Meeting   30        solo  true    30min       https://calendly.com/acme/30min
Team Standup        60        group true    standup     https://calendly.com/acme/standup
```

---

#### `calendly event-types show <uuid>`

```
calendly event-types show <uuid> [--json]
```

Accepts UUID, UUID prefix (8+ chars), name, or slug via resolver (Resolution E-04).

---

#### `calendly event-types available-times <uuid>`

```
calendly event-types available-times <uuid> --start <iso8601> --end <iso8601>
  [--date <YYYY-MM-DD>] [--today] [--tomorrow] [--this-week] [--json]
```

| Flag | Type | Default | Description |
|------|------|---------|-------------|
| `--start` | string | next 15-min boundary | Start of time range (Resolution E-06) |
| `--end` | string | start + 7 days | End of time range |
| `--date` | string | -- | Sets start/end to the given day in user's timezone |
| `--today` | flag | false | Shorthand for today |
| `--tomorrow` | flag | false | Shorthand for tomorrow |
| `--this-week` | flag | false | Shorthand for rest of week |

**Output example (table mode):**
```
START TIME                    SPOTS
Mar 12, 2:00 PM EST          1
Mar 12, 2:30 PM EST          1
Mar 12, 3:00 PM EST          3
Mar 13, 9:00 AM EST          1
```

---

#### `calendly one-off create`

```
calendly one-off create --name <name> --duration <min> --timezone <tz>
  --start-date <YYYY-MM-DD> --end-date <YYYY-MM-DD> --location-kind <kind>
  [--location <details>] [--additional-info <info>] [--co-host <uri>...]
  [--host <uri>] [--dry-run] [--yes] [--json]
```

| Flag | Required | Default | Description |
|------|----------|---------|-------------|
| `--name` | Yes | -- | Event type name |
| `--duration` | Yes | -- | Duration in minutes |
| `--timezone` | Yes | -- | IANA timezone |
| `--start-date` | Yes | -- | Booking window start (YYYY-MM-DD) |
| `--end-date` | Yes | -- | Booking window end (YYYY-MM-DD) |
| `--location-kind` | Yes | -- | Location type |
| `--location` | No | -- | Location details (required for physical/custom) |
| `--additional-info` | No | -- | Extra location info. Sent as `additonal_info` (API typo, Resolution H-09). |
| `--co-host` | No | -- | Co-host URI (repeatable) |
| `--host` | No | current user | Host URI |

---

### Group: Data

#### `calendly invitees list <event_uuid>`

```
calendly invitees list <event_uuid> [--count N] [--all] [--status <active|canceled>]
  [--email <email>] [--sort <field:dir>] [--json]
```

---

#### `calendly invitees show <invitee_uuid>`

```
calendly invitees show <invitee_uuid> [--json]
```

Returns InviteRecord (slim resource from `/invites/` path).

---

#### `calendly invitees book` (hidden alias)

```
calendly invitees book --event-type <uri> --start-time <iso8601> --name <name> --email <email>
  --timezone <tz> [options...] [--json]
```

Hidden from help. Uses POST /invitees. Prefer `calendly book`.

---

#### `calendly org invitations`

```
calendly org invitations [--status <pending|accepted|declined>] [--email <email>]
  [--count N] [--all] [--sort <field:dir>] [--json]
```

Org UUID auto-resolved from cached user profile.

---

#### `calendly activity-log list`

```
calendly activity-log list [--org <uri>] [--action <action>] [--actor <uri>]
  [--namespace <ns>] [--search <term>] [--since <iso8601>] [--until <iso8601>]
  [--sort <occurred_at:asc|occurred_at:desc>] [--count N] [--all] [--json]
```

| CLI Flag | API Parameter |
|----------|--------------|
| `--org` | `organization` (auto-populated from cache) |
| `--action` | `action` |
| `--actor` | `actor` |
| `--namespace` | `namespace` |
| `--search` | `search_term` |
| `--since` | `min_occurred_at` |
| `--until` | `max_occurred_at` |
| `--sort` | `sort` |
| `--count` | `count` |

---

### Group: System

#### `calendly me`

```
calendly me [--refresh] [--json]
```

Calls `GET /users/me`. Caches result for 24h. `--refresh` forces re-fetch.

**Output example (detail mode):**
```
Name:           John Doe
Email:          john@example.com
Timezone:       America/New_York
Scheduling URL: https://calendly.com/johndoe
Organization:   organizations/AAAA...
Slug:           johndoe
Locale:         en
Created:        Jan 2, 2019
```

---

#### `calendly config show`

Display current configuration.

```
calendly config show
```

#### `calendly config set <key> <value>`

Set a configuration value.

```
calendly config set token "your_token_here"
calendly config set timezone "America/Chicago"
calendly config set color false
calendly config set count 50
```

#### `calendly config path`

Print the config file path.

```
calendly config path
```

---

#### `calendly cache clear`

Clear all cached data.

```
calendly cache clear [--user] [--event-types] [--all]
```

#### `calendly cache show`

Show cache status (what is cached, when it expires).

```
calendly cache show
```

---

## 6. Build System

### CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.20)
project(calendly VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

configure_file(src/core/version.h.in src/core/version.h)

# --- FetchContent Dependencies ---
include(FetchContent)

FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)

FetchContent_Declare(cli11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG v2.4.2
)

FetchContent_Declare(tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG v3.4.0
)

FetchContent_MakeAvailable(json cli11 tomlplusplus)

# --- System Dependencies ---
find_package(CURL REQUIRED)
find_package(SQLite3 REQUIRED)

# --- Main Executable ---
add_executable(calendly
    src/main.cpp

    # Core infrastructure
    src/core/http_client.cpp
    src/core/rest_client.cpp
    src/core/auth.cpp
    src/core/config.cpp
    src/core/output.cpp
    src/core/color.cpp
    src/core/error.cpp
    src/core/filter.cpp
    src/core/cache.cpp
    src/core/resolver.cpp

    # Modules
    src/modules/users/api.cpp
    src/modules/users/commands.cpp
    src/modules/event_types/api.cpp
    src/modules/event_types/commands.cpp
    src/modules/events/api.cpp
    src/modules/events/commands.cpp
    src/modules/invitees/api.cpp
    src/modules/invitees/commands.cpp
    src/modules/organizations/api.cpp
    src/modules/organizations/commands.cpp
    src/modules/activity_log/api.cpp
    src/modules/activity_log/commands.cpp
    src/modules/one_off/api.cpp
    src/modules/one_off/commands.cpp
    src/modules/book/commands.cpp
    src/modules/config/commands.cpp
    src/modules/cache/commands.cpp
)

target_include_directories(calendly PRIVATE src ${CMAKE_BINARY_DIR}/src)

target_link_libraries(calendly PRIVATE
    nlohmann_json::nlohmann_json
    CLI11::CLI11
    tomlplusplus::tomlplusplus
    CURL::libcurl
    SQLite::SQLite3
)

target_compile_options(calendly PRIVATE
    -Wall -Wextra -Wpedantic -Wshadow
)

install(TARGETS calendly DESTINATION bin)

# --- Testing ---
enable_testing()
add_subdirectory(tests)
```

### Dependencies Summary

| Dependency | Version | Method | Purpose |
|------------|---------|--------|---------|
| nlohmann/json | v3.11.3 | FetchContent | JSON parsing/serialization |
| CLI11 | v2.4.2 | FetchContent | Command-line parsing |
| tomlplusplus | v3.4.0 | FetchContent | TOML config file parsing |
| libcurl | system | find_package | HTTP client |
| SQLite3 | system | find_package | Cache storage (future: structured cache) |
| GoogleTest | v1.15.2 | FetchContent (in tests/) | Unit/integration testing |

### version.h.in

```cpp
// src/core/version.h.in
#pragma once
#define CALENDLY_VERSION "@PROJECT_VERSION@"
```

---

## 7. Implementation Order

### Phase 1: Core Infrastructure (11 files)

Build the foundation that all modules depend on.

| # | File | Description |
|---|------|-------------|
| 1 | `src/core/types.h` | CursorPageInfo, CursorPage<T>, enums, Tracking, Q&A, Cancellation |
| 2 | `src/core/error.h` / `error.cpp` | CalendlyError, check_http_status, format_error |
| 3 | `src/core/color.h` / `color.cpp` | ANSI colors, pipe detection |
| 4 | `src/core/output.h` / `output.cpp` | TableRenderer, DetailRenderer, JSON/CSV output |
| 5 | `src/core/http_client.h` / `http_client.cpp` | libcurl RAII wrapper, rate limit headers |
| 6 | `src/core/auth.h` / `auth.cpp` | Token resolution (env > config) |
| 7 | `src/core/config.h` / `config.cpp` | TOML config management |
| 8 | `src/core/rest_client.h` / `rest_client.cpp` | URL building, auth injection, JSON parsing, retry |
| 9 | `src/core/filter.h` / `filter.cpp` | UUID extraction, URI building, timezone validation |
| 10 | `src/core/paginator.h` | CursorPaginator<T> template |
| 11 | `src/core/version.h.in` | Version macro |

**Exit criteria:** RestClient can make authenticated GET/POST requests to `api.calendly.com` and parse JSON responses. Errors throw CalendlyError with title/message.

---

### Phase 2: Users Module (proves pattern end-to-end)

The simplest module. Validates the full model -> api -> commands pipeline.

| # | File | Description |
|---|------|-------------|
| 1 | `src/modules/users/model.h` | User struct + from_json |
| 2 | `src/modules/users/api.h` / `api.cpp` | GET /users/me |
| 3 | `src/modules/users/commands.h` / `commands.cpp` | `calendly me` command |

**Exit criteria:** `calendly me` displays the authenticated user's profile in table and JSON modes.

---

### Phase 3: Event Types Module

| # | File | Description |
|---|------|-------------|
| 1 | `src/modules/event_types/model.h` | EventType, EventTypeProfile, Location, CustomQuestion, AvailableTime |
| 2 | `src/modules/event_types/api.h` / `api.cpp` | GET /event_types, GET /event_types/{uuid}, GET /event_type_available_times |
| 3 | `src/modules/event_types/commands.h` / `commands.cpp` | `event-types list`, `show`, `available-times` |

**Exit criteria:** All three event-types commands work with pagination, `--all`, `--json`, `--active` flags.

---

### Phase 4: Events Module (list, show, create, cancel)

| # | File | Description |
|---|------|-------------|
| 1 | `src/modules/events/model.h` | ScheduledEvent, InviteesCounter, Create/Cancel request/response types |
| 2 | `src/modules/events/api.h` / `api.cpp` | GET /scheduled_events, GET /scheduled_events/{uuid}, POST /scheduled_events, POST /scheduled_events/{uuid}/cancellation |
| 3 | `src/modules/events/commands.h` / `commands.cpp` | `events list`, `events show`, `events create` (hidden), `events cancel` |

**Exit criteria:** Can list upcoming events, show event detail, create a booking, and cancel an event. `--today`, `--tomorrow`, `--this-week` shorthands work. Confirmation prompt works for cancel.

---

### Phase 5: Invitees Module

| # | File | Description |
|---|------|-------------|
| 1 | `src/modules/invitees/model.h` | Invitee, InviteRecord, BookingRequest, BookingResponse |
| 2 | `src/modules/invitees/api.h` / `api.cpp` | POST /invitees, GET /.../invitees, GET /invitees/{uuid} |
| 3 | `src/modules/invitees/commands.h` / `commands.cpp` | `invitees book` (hidden), `invitees list`, `invitees show` |

**Exit criteria:** All three invitees commands work. InviteRecord (from GET /invitees/{uuid}) is correctly differentiated from Invitee (Resolution B-04).

---

### Phase 6: Organizations + Activity Log + One-Off

| # | File | Description |
|---|------|-------------|
| 1 | `src/modules/organizations/model.h` | OrganizationInvitation |
| 2 | `src/modules/organizations/api.h` / `api.cpp` | GET /organizations/{uuid}/invitations |
| 3 | `src/modules/organizations/commands.h` / `commands.cpp` | `org invitations` |
| 4 | `src/modules/activity_log/model.h` | ActivityLogEntry, Actor, ActivityLogResponse |
| 5 | `src/modules/activity_log/api.h` / `api.cpp` | GET /activity_log_entries |
| 6 | `src/modules/activity_log/commands.h` / `commands.cpp` | `activity-log list` |
| 7 | `src/modules/one_off/model.h` | OneOffEventTypeRequest/Response, DateSetting, OneOffLocation |
| 8 | `src/modules/one_off/api.h` / `api.cpp` | POST /one_off_event_types |
| 9 | `src/modules/one_off/commands.h` / `commands.cpp` | `one-off create` |

**Exit criteria:** All three modules operational. Activity log handles the extended response format (Resolution H-02). One-off sends `additonal_info` (API typo, Resolution H-09).

---

### Phase 7: Config + Cache Modules

| # | File | Description |
|---|------|-------------|
| 1 | `src/core/cache.h` / `cache.cpp` | JSON file cache with TTL |
| 2 | `src/core/resolver.h` / `resolver.cpp` | Name-to-URI resolution |
| 3 | `src/modules/config/commands.h` / `commands.cpp` | `config show`, `config set`, `config path` |
| 4 | `src/modules/cache/commands.h` / `commands.cpp` | `cache clear`, `cache show` |

**Exit criteria:** User profile cached for 24h. Event types cached for 1h. Resolver can match event types by name, slug, UUID, UUID prefix. `--no-cache` and `--refresh` flags work. First-run auto-caching prints status to stderr (Resolution F-02).

---

### Phase 8: Composite Book Command

| # | File | Description |
|---|------|-------------|
| 1 | `src/modules/book/commands.h` / `commands.cpp` | `calendly book` composite command |

**Exit criteria:** `calendly book --type "30 Minute Meeting" --name "Jane" --email "jane@example.com"` resolves the event type, fetches available times, picks the first slot (with `--first`), and books it. Interactive mode prompts for time selection. `--dry-run` shows the request without sending.

---

### Phase 9: main.cpp Assembly + Build

| # | File | Description |
|---|------|-------------|
| 1 | `src/main.cpp` | CLI11 app setup, all command registration, global flags, banner |
| 2 | `CMakeLists.txt` | Build configuration |
| 3 | `tests/CMakeLists.txt` | Test build configuration |

**Exit criteria:** `cmake --build .` produces a working `calendly` binary. All commands accessible. Grouped help output (Scheduling, Data, System). `--version` shows banner and version.

### main.cpp Structure

```cpp
// src/main.cpp

#include <CLI/CLI.hpp>
#include <curl/curl.h>

#include "core/color.h"
#include "core/error.h"
#include "core/output.h"
#include "core/version.h"
#include "modules/users/commands.h"
#include "modules/event_types/commands.h"
#include "modules/events/commands.h"
#include "modules/invitees/commands.h"
#include "modules/organizations/commands.h"
#include "modules/activity_log/commands.h"
#include "modules/one_off/commands.h"
#include "modules/book/commands.h"
#include "modules/config/commands.h"
#include "modules/cache/commands.h"

int main(int argc, char** argv) {
    // Bare `calendly` -- show splash
    if (argc == 1) { /* banner */ return 0; }

    curl_global_init(CURL_GLOBAL_DEFAULT);

    CLI::App app{"Calendly CLI for humans and AI agents"};
    app.require_subcommand();
    app.fallthrough();

    // Global flags
    bool json_output = false, csv_output = false, no_color = false, no_cache = false, verbose = false;
    app.add_flag("--json", json_output, "Output raw JSON");
    app.add_flag("--csv", csv_output, "Output CSV");
    app.add_flag("--no-color", no_color, "Disable colors");
    app.add_flag("--no-cache", no_cache, "Bypass cache");
    app.add_flag("--verbose", verbose, "Show debug info");

    app.parse_complete_callback([&]() {
        if (json_output) set_output_format(OutputFormat::Json);
        if (csv_output) set_output_format(OutputFormat::Csv);
        if (no_color) color::set_enabled(false);
    });

    // Register modules -- Scheduling
    book_commands::register_commands(app);           // calendly book
    events_commands::register_commands(app);          // calendly events {list,show,create,cancel}
    event_types_commands::register_commands(app);     // calendly event-types {list,show,available-times}
    one_off_commands::register_commands(app);         // calendly one-off create

    // Register modules -- Data
    invitees_commands::register_commands(app);        // calendly invitees {book,list,show}
    organizations_commands::register_commands(app);   // calendly org invitations
    activity_log_commands::register_commands(app);    // calendly activity-log list

    // Register modules -- System
    users_commands::register_commands(app);           // calendly me
    config_commands::register_commands(app);          // calendly config {show,set,path}
    cache_commands::register_commands(app);           // calendly cache {clear,show}

    // Group subcommands
    app.get_subcommand("book")->group("Scheduling");
    app.get_subcommand("events")->group("Scheduling");
    app.get_subcommand("event-types")->group("Scheduling");
    app.get_subcommand("one-off")->group("Scheduling");

    app.get_subcommand("invitees")->group("Data");
    app.get_subcommand("org")->group("Data");
    app.get_subcommand("activity-log")->group("Data");

    app.get_subcommand("me")->group("System");
    app.get_subcommand("config")->group("System");
    app.get_subcommand("cache")->group("System");

    // Parse and execute
    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        curl_global_cleanup();
        return app.exit(e);
    } catch (const CalendlyError& e) {
        curl_global_cleanup();
        return 1;
    } catch (const std::exception& e) {
        print_error(std::string("Unexpected error: ") + e.what());
        curl_global_cleanup();
        return 1;
    }

    curl_global_cleanup();
    return 0;
}
```

---

## 8. Testing Strategy

### Framework

GoogleTest v1.15.2 via FetchContent.

```cmake
# tests/CMakeLists.txt
include(FetchContent)
FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
)
FetchContent_MakeAvailable(googletest)

enable_testing()
include(GoogleTest)

# Unit tests
add_executable(calendly_tests
    test_types.cpp
    test_filter.cpp
    test_models.cpp
    test_cache.cpp
)

target_link_libraries(calendly_tests PRIVATE
    GTest::gtest_main
    nlohmann_json::nlohmann_json
    CLI11::CLI11
    tomlplusplus::tomlplusplus
    CURL::libcurl
)

gtest_discover_tests(calendly_tests)
```

### Test Categories

#### Unit Tests (67 tests, 100% pass rate)

| Test File | What It Tests |
|-----------|---------------|
| `test_types.cpp` | CursorPageInfo from_json, Tracking/Q&A parsing, Cancellation empty object handling, EventStatus/InviteeStatus/LocationKind enum parsing with aliases |
| `test_filter.cpp` | extract_uuid, build_uri, timezone validation, sort validation, count validation (min 1), build_query_string, format_relative_time, short_uuid, strip_base_url |
| `test_models.cpp` | User from_json (complete, nullable avatar, missing fields), EventType from_json (complete, nullable, custom questions), Location from_json, AvailableTime from_json |
| `test_cache.cpp` | Cache read/write, TTL freshness, invalidation (clear_all, clear_user, clear_event_types), --no-cache bypass, status reporting, persistence across instances |

#### E2E Test Plan

Manual and scripted tests against the live Calendly API (Phase 3 QA).

| Test | Command | Validates |
|------|---------|-----------|
| Auth | `calendly me` | Token works, user profile displayed |
| List event types | `calendly event-types list --active --json` | Pagination, active filter |
| Show event type | `calendly event-types show <uuid> --json` | Detail fields, locations, custom questions |
| Available times | `calendly event-types available-times <uuid> --today` | Smart defaults, time slots returned |
| Book a meeting | `calendly book --type "15min" --name "Test" --email "test@test.com" --first --yes` | Full composite flow |
| List events | `calendly events list --today --json` | Date filters, event status spelling (G-02) |
| Show event | `calendly events show <uuid> --json` | Response wrapper shape (G-01) |
| Cancel event | `calendly events cancel <uuid> --reason "testing" --yes` | Cancellation flow |
| List invitees | `calendly invitees list <event_uuid> --json` | Invitee full object, pagination |
| Show invitee | `calendly invitees show <uuid> --json` | InviteRecord distinct type (B-04) |
| Org invitations | `calendly org invitations --status pending` | Auto org UUID resolution |
| Activity log | `calendly activity-log list --count 5 --json` | Extended response, org auto-populate |
| One-off create | `calendly one-off create --name "Test" --duration 30 --timezone "UTC" --start-date 2026-04-01 --end-date 2026-04-03 --location-kind zoom_conference --yes` | API typo field (G-07), response schema (G-08) |
| Cache | `calendly me && calendly me` (second is instant) | Cache hit, 24h TTL |
| No cache | `calendly me --no-cache` | Bypasses cache |
| Dry run | `calendly book --type "15min" --name "Test" --email "test@test.com" --first --dry-run` | Shows request, does not send |

### QA Verification Items (from RESOLUTIONS.md Section G)

All 13 deferred-to-QA items must be verified during E2E testing:

| ID | Item | Working Assumption |
|----|------|--------------------|
| G-01 | GET event response wrapper | `{"resource": {...}}` |
| G-02 | Event status spelling | `"cancelled"` (double l) |
| G-03 | POST /invitees response shape | Flat |
| G-04 | POST /invitees status code | 200 |
| G-05 | first_name/last_name required | Optional |
| G-06 | organization param for activity log | Required |
| G-07 | `additonal_info` typo | API expects the typo |
| G-08 | One-off response schema | All fields optional |
| G-09 | location field type on GET event | String or null |
| G-10 | "virtual" location kind | Artifact (exclude) |
| G-11 | is_paid in list response | Not included in list |
| G-12 | invitees_remaining in available times | Present but unconfirmed |
| G-13 | Pagination URL in activity log | Use whatever API returns |

### Coverage Target

**Minimum: 80%** line coverage across all source files.

**Critical coverage areas (must be 100%):**
- Enum parsing (EventStatus, InviteeStatus, LocationKind) with alias resolution
- from_json for all model structs
- Error handling (all HTTP status codes)
- UUID extraction and URI building
- Cache TTL logic

---

*This document is the single source of truth for all calendly-cli implementation work. Every struct, every field, every command, every flag is documented here. Disputes and ambiguities should reference the relevant RESOLUTIONS.md resolution ID.*
