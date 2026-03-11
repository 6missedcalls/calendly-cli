#pragma once
#include <optional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "core/types.h"

using json = nlohmann::json;

// --- InviteesCounter ---

struct InviteesCounter {
    int total = 0;
    int confirmed = 0;
    int declined = 0;
};

inline void from_json(const json& j, InviteesCounter& ic) {
    ic.total = j.value("total", 0);
    ic.confirmed = j.value("confirmed", 0);
    ic.declined = j.value("declined", 0);
}

// --- ScheduledEvent ---

struct ScheduledEvent {
    std::string uri;
    std::optional<std::string> uuid;
    std::string created_at;
    std::string start_time;
    std::string end_time;
    std::string status;          // "confirmed" or "cancelled" (British)
    std::string event_type;      // Event type URI
    std::optional<std::string> location;
    InviteesCounter invitees_counter;
    std::vector<QuestionAndAnswer> questions_and_answers;
};

inline void from_json(const json& j, ScheduledEvent& ev) {
    ev.uri = j.value("uri", "");
    ev.created_at = j.value("created_at", "");
    ev.start_time = j.value("start_time", "");
    ev.end_time = j.value("end_time", "");
    ev.status = j.value("status", "");
    ev.event_type = j.value("event_type", "");

    if (j.contains("location") && !j["location"].is_null()) {
        if (j["location"].is_string()) {
            ev.location = j["location"].get<std::string>();
        } else if (j["location"].is_object() && j["location"].contains("location")) {
            if (!j["location"]["location"].is_null()) {
                ev.location = j["location"]["location"].get<std::string>();
            }
        }
    }

    if (j.contains("invitees_counter") && j["invitees_counter"].is_object()) {
        ev.invitees_counter = j["invitees_counter"].get<InviteesCounter>();
    }

    if (j.contains("questions_and_answers") && j["questions_and_answers"].is_array()) {
        ev.questions_and_answers = j["questions_and_answers"].get<std::vector<QuestionAndAnswer>>();
    }

    // Extract uuid: prefer explicit field, fall back to last path segment of uri
    if (j.contains("uuid") && !j["uuid"].is_null()) {
        ev.uuid = j["uuid"].get<std::string>();
    } else if (!ev.uri.empty()) {
        const auto pos = ev.uri.rfind('/');
        if (pos != std::string::npos && pos + 1 < ev.uri.size()) {
            ev.uuid = ev.uri.substr(pos + 1);
        }
    }
}

// --- InviteeRequest ---

struct InviteeRequest {
    std::string name;
    std::optional<std::string> first_name;
    std::optional<std::string> last_name;
    std::string email;
    std::string timezone;
    std::optional<std::string> text_reminder_number;
};

inline json to_json_obj(const InviteeRequest& req) {
    json j = json::object();
    j["name"] = req.name;
    j["email"] = req.email;
    j["timezone"] = req.timezone;
    if (req.first_name) { j["first_name"] = *req.first_name; }
    if (req.last_name) { j["last_name"] = *req.last_name; }
    if (req.text_reminder_number) { j["text_reminder_number"] = *req.text_reminder_number; }
    return j;
}

// --- LocationRequest ---

struct LocationRequest {
    std::string kind;
    std::optional<std::string> location;
};

inline json to_json_obj(const LocationRequest& req) {
    json j = json::object();
    j["kind"] = req.kind;
    if (req.location) { j["location"] = *req.location; }
    return j;
}

// --- CreateEventRequest ---

struct CreateEventRequest {
    std::string event_type;      // Full event type URI
    std::string start_time;      // ISO 8601
    InviteeRequest invitee;
    std::optional<LocationRequest> location;
    std::vector<QuestionAndAnswer> questions_and_answers;
    std::optional<Tracking> tracking;
    std::vector<std::string> event_guests;
};

inline json to_json_obj(const CreateEventRequest& req) {
    json j = json::object();
    j["event_type"] = req.event_type;
    j["start_time"] = req.start_time;
    j["invitee"] = to_json_obj(req.invitee);

    if (req.location) {
        j["location"] = to_json_obj(*req.location);
    }

    if (!req.questions_and_answers.empty()) {
        json qa_arr = json::array();
        for (const auto& qa : req.questions_and_answers) {
            qa_arr.push_back(to_json_obj(qa));
        }
        j["questions_and_answers"] = qa_arr;
    }

    if (req.tracking) {
        j["tracking"] = to_json_obj(*req.tracking);
    }

    if (!req.event_guests.empty()) {
        json guests = json::array();
        for (const auto& guest : req.event_guests) {
            guests.push_back(guest);
        }
        j["event_guests"] = guests;
    }

    return j;
}

// --- CreateEventResponse ---
// IMPORTANT: `resource` is a URI string, NOT an object (Resolution B-03)

struct CreateEventResponse {
    std::string resource;        // URI string to the created event
    json invitee;                // Full invitee object (raw JSON for now)
};

inline void from_json(const json& j, CreateEventResponse& resp) {
    if (j.contains("resource") && !j["resource"].is_null()) {
        if (j["resource"].is_string()) {
            resp.resource = j["resource"].get<std::string>();
        } else if (j["resource"].is_object() && j["resource"].contains("uri")) {
            resp.resource = j["resource"]["uri"].get<std::string>();
        }
    }

    if (j.contains("invitee") && !j["invitee"].is_null()) {
        resp.invitee = j["invitee"];
    }
}

// --- CancellationRequest ---

struct CancellationRequest {
    std::optional<std::string> reason;
};

inline json to_json_obj(const CancellationRequest& req) {
    json j = json::object();
    if (req.reason) { j["reason"] = *req.reason; }
    return j;
}

// --- CancellationResponse ---

struct CancellationResponse {
    std::string canceled_by;
    std::optional<std::string> reason;
};

inline void from_json(const json& j, CancellationResponse& resp) {
    resp.canceled_by = j.value("canceled_by", "");
    if (j.contains("reason") && !j["reason"].is_null()) {
        resp.reason = j["reason"].get<std::string>();
    }
}
