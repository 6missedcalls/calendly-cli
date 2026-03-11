#pragma once
#include <optional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "core/types.h"

using json = nlohmann::json;

// Full invitee object returned by POST /invitees, POST /scheduled_events,
// and GET /scheduled_events/{uuid}/invitees (list).
struct Invitee {
    std::string uri;
    std::string email;
    std::optional<std::string> first_name;
    std::optional<std::string> last_name;
    std::string name;
    std::string status;          // "active" or "canceled" (American single 'l')
    std::vector<QuestionAndAnswer> questions_and_answers;
    std::optional<std::string> timezone;
    std::string event;           // Scheduled event URI
    std::string created_at;
    std::string updated_at;
    Tracking tracking;           // Always present, may be {} (Resolution H-03)
    std::optional<std::string> text_reminder_number;
    bool rescheduled = false;
    std::optional<std::string> old_invitee;
    std::optional<std::string> new_invitee;
    std::string cancel_url;
    std::string reschedule_url;
    std::optional<std::string> routing_form_submission;
    Cancellation cancellation;   // Always present, may be {} (Resolution H-03)
    std::optional<json> payment;
    std::optional<json> no_show;
    std::optional<json> reconfirmation;
    std::optional<std::string> scheduling_method;
    std::optional<std::string> invitee_scheduled_by;
};

inline void from_json(const json& j, Invitee& inv) {
    inv.uri = j.value("uri", "");
    inv.email = j.value("email", "");
    if (j.contains("first_name") && !j["first_name"].is_null()) inv.first_name = j["first_name"].get<std::string>();
    if (j.contains("last_name") && !j["last_name"].is_null()) inv.last_name = j["last_name"].get<std::string>();
    inv.name = j.value("name", "");
    inv.status = j.value("status", "active");
    if (j.contains("questions_and_answers") && j["questions_and_answers"].is_array()) {
        inv.questions_and_answers = j["questions_and_answers"].get<std::vector<QuestionAndAnswer>>();
    }
    if (j.contains("timezone") && !j["timezone"].is_null()) inv.timezone = j["timezone"].get<std::string>();
    inv.event = j.value("event", "");
    inv.created_at = j.value("created_at", "");
    inv.updated_at = j.value("updated_at", "");
    if (j.contains("tracking")) inv.tracking = j["tracking"].get<Tracking>();
    if (j.contains("text_reminder_number") && !j["text_reminder_number"].is_null()) inv.text_reminder_number = j["text_reminder_number"].get<std::string>();
    inv.rescheduled = j.value("rescheduled", false);
    if (j.contains("old_invitee") && !j["old_invitee"].is_null()) inv.old_invitee = j["old_invitee"].get<std::string>();
    if (j.contains("new_invitee") && !j["new_invitee"].is_null()) inv.new_invitee = j["new_invitee"].get<std::string>();
    inv.cancel_url = j.value("cancel_url", "");
    inv.reschedule_url = j.value("reschedule_url", "");
    if (j.contains("routing_form_submission") && !j["routing_form_submission"].is_null()) inv.routing_form_submission = j["routing_form_submission"].get<std::string>();
    if (j.contains("cancellation")) inv.cancellation = j["cancellation"].get<Cancellation>();
    if (j.contains("payment") && !j["payment"].is_null()) inv.payment = j["payment"];
    if (j.contains("no_show") && !j["no_show"].is_null()) inv.no_show = j["no_show"];
    if (j.contains("reconfirmation") && !j["reconfirmation"].is_null()) inv.reconfirmation = j["reconfirmation"];
    if (j.contains("scheduling_method") && !j["scheduling_method"].is_null()) inv.scheduling_method = j["scheduling_method"].get<std::string>();
    if (j.contains("invitee_scheduled_by") && !j["invitee_scheduled_by"].is_null()) inv.invitee_scheduled_by = j["invitee_scheduled_by"].get<std::string>();
}

// Distinct resource type returned by GET /invitees/{invitee_uuid} (Resolution B-04).
// Uses /invites/ path prefix. Slim record with only 8 fields.
struct InviteRecord {
    std::string uri;             // https://api.calendly.com/invites/BBBB...
    std::string invitee_uuid;
    std::string status;          // active, canceled, accepted, declined
    std::string created_at;
    std::string start_time;
    std::string end_time;
    std::string event;           // Scheduled event URI
    std::string invitation_secret;  // Sensitive token
};

inline void from_json(const json& j, InviteRecord& ir) {
    ir.uri = j.value("uri", "");
    ir.invitee_uuid = j.value("invitee_uuid", "");
    ir.status = j.value("status", "");
    ir.created_at = j.value("created_at", "");
    ir.start_time = j.value("start_time", "");
    ir.end_time = j.value("end_time", "");
    ir.event = j.value("event", "");
    ir.invitation_secret = j.value("invitation_secret", "");
}

// Request body for POST /invitees is identical to CreateEventRequest
// Use the events/model.h CreateEventRequest type when building requests.

// Response from POST /invitees (Resolution B-01)
// Working assumption: flat response (no resource/invitee wrapper).
struct BookingResponse {
    std::string event;
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

inline void from_json(const json& j, BookingResponse& r) {
    r.event = j.value("event", "");
    r.status = j.value("status", "");
    if (j.contains("timezone") && !j["timezone"].is_null()) r.timezone = j["timezone"].get<std::string>();
    r.cancel_url = j.value("cancel_url", "");
    r.reschedule_url = j.value("reschedule_url", "");
    if (j.contains("questions_and_answers") && j["questions_and_answers"].is_array()) {
        r.questions_and_answers = j["questions_and_answers"].get<std::vector<QuestionAndAnswer>>();
    }
    if (j.contains("text_reminder_number") && !j["text_reminder_number"].is_null()) r.text_reminder_number = j["text_reminder_number"].get<std::string>();
    r.created_at = j.value("created_at", "");
    r.updated_at = j.value("updated_at", "");
    r.rescheduled = j.value("rescheduled", false);
    if (j.contains("old_invitee") && !j["old_invitee"].is_null()) r.old_invitee = j["old_invitee"].get<std::string>();
    if (j.contains("new_invitee") && !j["new_invitee"].is_null()) r.new_invitee = j["new_invitee"].get<std::string>();
    r.uri = j.value("uri", "");
    r.email = j.value("email", "");
    r.name = j.value("name", "");
    if (j.contains("first_name") && !j["first_name"].is_null()) r.first_name = j["first_name"].get<std::string>();
    if (j.contains("last_name") && !j["last_name"].is_null()) r.last_name = j["last_name"].get<std::string>();
}
