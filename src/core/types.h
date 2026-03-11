#pragma once

#include <algorithm>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

// --- Pagination ---

struct CursorPageInfo {
    int count = 0;
    std::optional<std::string> next_page;
    std::optional<std::string> previous_page;
    std::optional<std::string> next_page_token;
    std::optional<std::string> previous_page_token;
};

inline void from_json(const json& j, CursorPageInfo& p) {
    p.count = j.value("count", 0);

    if (j.contains("next_page") && !j["next_page"].is_null()) {
        p.next_page = j["next_page"].get<std::string>();
    }
    if (j.contains("previous_page") && !j["previous_page"].is_null()) {
        p.previous_page = j["previous_page"].get<std::string>();
    }
    if (j.contains("next_page_token") && !j["next_page_token"].is_null()) {
        p.next_page_token = j["next_page_token"].get<std::string>();
    }
    if (j.contains("previous_page_token") && !j["previous_page_token"].is_null()) {
        p.previous_page_token = j["previous_page_token"].get<std::string>();
    }
}

template<typename T>
struct CursorPage {
    std::vector<T> collection;
    CursorPageInfo pagination;
};

template<typename T>
void from_json(const json& j, CursorPage<T>& page) {
    if (j.contains("collection")) {
        page.collection = j["collection"].get<std::vector<T>>();
    }
    if (j.contains("pagination")) {
        page.pagination = j["pagination"].get<CursorPageInfo>();
    }
}

// --- Tracking Object ---

struct Tracking {
    std::optional<std::string> utm_campaign;
    std::optional<std::string> utm_source;
    std::optional<std::string> utm_medium;
    std::optional<std::string> utm_content;
    std::optional<std::string> utm_term;
    std::optional<std::string> salesforce_uuid;
};

inline void from_json(const json& j, Tracking& t) {
    if (j.contains("utm_campaign") && !j["utm_campaign"].is_null()) {
        t.utm_campaign = j["utm_campaign"].get<std::string>();
    }
    if (j.contains("utm_source") && !j["utm_source"].is_null()) {
        t.utm_source = j["utm_source"].get<std::string>();
    }
    if (j.contains("utm_medium") && !j["utm_medium"].is_null()) {
        t.utm_medium = j["utm_medium"].get<std::string>();
    }
    if (j.contains("utm_content") && !j["utm_content"].is_null()) {
        t.utm_content = j["utm_content"].get<std::string>();
    }
    if (j.contains("utm_term") && !j["utm_term"].is_null()) {
        t.utm_term = j["utm_term"].get<std::string>();
    }
    if (j.contains("salesforce_uuid") && !j["salesforce_uuid"].is_null()) {
        t.salesforce_uuid = j["salesforce_uuid"].get<std::string>();
    }
}

inline json to_json_obj(const Tracking& t) {
    json j = json::object();
    if (t.utm_campaign) { j["utm_campaign"] = *t.utm_campaign; }
    if (t.utm_source) { j["utm_source"] = *t.utm_source; }
    if (t.utm_medium) { j["utm_medium"] = *t.utm_medium; }
    if (t.utm_content) { j["utm_content"] = *t.utm_content; }
    if (t.utm_term) { j["utm_term"] = *t.utm_term; }
    if (t.salesforce_uuid) { j["salesforce_uuid"] = *t.salesforce_uuid; }
    return j;
}

// --- Question And Answer ---

struct QuestionAndAnswer {
    std::string question;
    std::string answer;
    std::optional<int> position;
};

inline void from_json(const json& j, QuestionAndAnswer& qa) {
    qa.question = j.value("question", "");
    qa.answer = j.value("answer", "");
    if (j.contains("position") && !j["position"].is_null()) {
        qa.position = j["position"].get<int>();
    }
}

inline json to_json_obj(const QuestionAndAnswer& qa) {
    json j = json::object();
    j["question"] = qa.question;
    j["answer"] = qa.answer;
    if (qa.position) { j["position"] = *qa.position; }
    return j;
}

// --- Cancellation Object ---
// Always present in invitee responses (never null/absent).
// May be empty object {} when no cancellation has occurred (Resolution H-03).

struct Cancellation {
    std::optional<std::string> canceled_by;
    std::optional<std::string> reason;
};

inline void from_json(const json& j, Cancellation& c) {
    if (j.contains("canceled_by") && !j["canceled_by"].is_null()) {
        c.canceled_by = j["canceled_by"].get<std::string>();
    }
    if (j.contains("reason") && !j["reason"].is_null()) {
        c.reason = j["reason"].get<std::string>();
    }
}

// --- Error Response ---

struct CalendlyApiError {
    std::string title;
    std::string message;
};

inline void from_json(const json& j, CalendlyApiError& e) {
    e.title = j.value("title", "");
    e.message = j.value("message", "");
}

// --- Event Status (Resolution C-02) ---
// Uses "cancelled" (British spelling, double 'l').
// Implementation accepts both "cancelled" and "canceled" defensively.

enum class EventStatus { Confirmed, Cancelled, Unknown };

namespace detail {

inline std::string to_lower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return result;
}

} // namespace detail

inline EventStatus parse_event_status(const std::string& s) {
    const auto lower = detail::to_lower(s);
    if (lower == "confirmed") { return EventStatus::Confirmed; }
    if (lower == "cancelled" || lower == "canceled") { return EventStatus::Cancelled; }
    return EventStatus::Unknown;
}

inline std::string to_string(EventStatus s) {
    switch (s) {
        case EventStatus::Confirmed: return "confirmed";
        case EventStatus::Cancelled: return "cancelled";
        case EventStatus::Unknown:   return "unknown";
    }
    return "unknown";
}

// --- Invitee Status (Resolution C-01) ---
// Uses "canceled" (American spelling, single 'l').
// Superset across all endpoints.

enum class InviteeStatus { Active, Canceled, Accepted, Declined, Unknown };

inline InviteeStatus parse_invitee_status(const std::string& s) {
    const auto lower = detail::to_lower(s);
    if (lower == "active") { return InviteeStatus::Active; }
    if (lower == "canceled" || lower == "cancelled") { return InviteeStatus::Canceled; }
    if (lower == "accepted") { return InviteeStatus::Accepted; }
    if (lower == "declined") { return InviteeStatus::Declined; }
    return InviteeStatus::Unknown;
}

inline std::string to_string(InviteeStatus s) {
    switch (s) {
        case InviteeStatus::Active:   return "active";
        case InviteeStatus::Canceled: return "canceled";
        case InviteeStatus::Accepted: return "accepted";
        case InviteeStatus::Declined: return "declined";
        case InviteeStatus::Unknown:  return "unknown";
    }
    return "unknown";
}

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

inline LocationKind parse_location_kind(const std::string& s) {
    const auto lower = detail::to_lower(s);

    // Canonical values
    if (lower == "physical") { return LocationKind::Physical; }
    if (lower == "ask_invitee") { return LocationKind::AskInvitee; }
    if (lower == "custom") { return LocationKind::Custom; }
    if (lower == "zoom_conference") { return LocationKind::ZoomConference; }
    if (lower == "google_conference") { return LocationKind::GoogleConference; }
    if (lower == "microsoft_teams_conference") { return LocationKind::MicrosoftTeamsConference; }
    if (lower == "outbound_call") { return LocationKind::OutboundCall; }
    if (lower == "inbound_call") { return LocationKind::InboundCall; }
    if (lower == "gotomeeting") { return LocationKind::GoToMeeting; }
    if (lower == "webex") { return LocationKind::Webex; }

    // Alias resolution (Resolution C-03)
    if (lower == "zoom") { return LocationKind::ZoomConference; }
    if (lower == "google_hangouts") { return LocationKind::GoogleConference; }
    if (lower == "microsoft_teams") { return LocationKind::MicrosoftTeamsConference; }
    if (lower == "phone_call") { return LocationKind::OutboundCall; }
    // "virtual" -> Unknown (no distinct handling needed)

    return LocationKind::Unknown;
}

inline std::string to_string(LocationKind k) {
    switch (k) {
        case LocationKind::Physical:                 return "physical";
        case LocationKind::AskInvitee:               return "ask_invitee";
        case LocationKind::Custom:                   return "custom";
        case LocationKind::ZoomConference:           return "zoom_conference";
        case LocationKind::GoogleConference:         return "google_conference";
        case LocationKind::MicrosoftTeamsConference: return "microsoft_teams_conference";
        case LocationKind::OutboundCall:             return "outbound_call";
        case LocationKind::InboundCall:              return "inbound_call";
        case LocationKind::GoToMeeting:              return "gotomeeting";
        case LocationKind::Webex:                    return "webex";
        case LocationKind::Unknown:                  return "unknown";
    }
    return "unknown";
}
