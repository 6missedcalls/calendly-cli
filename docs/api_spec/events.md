# Calendly Scheduled Events API Specification

> Base URL: `https://api.calendly.com`
> Authentication: Bearer token (OAuth 2.0) via `Authorization: Bearer <token>` header.
> Pagination: Cursor-based pagination for list endpoints. Single-resource endpoints return no pagination.

---

## Table of Contents

1. [Get Event](#1-get-event)
2. [List Scheduled Events](#2-list-scheduled-events)
3. [Create Scheduled Event](#3-create-scheduled-event)
4. [Cancel Scheduled Event](#4-cancel-scheduled-event)
5. [Shared Schemas](#5-shared-schemas)
6. [Error Responses](#6-error-responses)
7. [Rate Limits](#7-rate-limits)
8. [Relationship: POST /scheduled_events vs POST /invitees](#8-relationship-post-scheduled_events-vs-post-invitees)

---

## 1. Get Event

Retrieve complete details of a specific scheduled event by its UUID.

| Property | Value |
|----------|-------|
| **Method** | `GET` |
| **Path** | `/scheduled_events/{event_uuid}` |
| **CLI Command** | `calendly events show <uuid>` |

### Path Parameters

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `event_uuid` | string | **Yes** | The UUID of the scheduled event. Example: `"eventuuid123"` |

### Request Headers

| Header | Type | Required | Default | Allowed Values |
|--------|------|----------|---------|----------------|
| `Authorization` | string | **Yes** | -- | `Bearer <token>` |
| `Content-Type` | string | No | `application/json` | `application/json` |

### Response `200 OK`

`[DEFERRED TO QA]` Verify against live API whether the response uses the `{"resource": {...}}` wrapper shown below. If the response is truly flat (no resource wrapper), the implementation must handle both shapes defensively.

```json
{
  "resource": {
    "uri": "https://api.calendly.com/scheduled_events/eventuuid123",
    "uuid": "eventuuid123",
    "created_at": "2020-01-01T12:00:00.000Z",
    "start_time": "2020-01-01T13:00:00.000Z",
    "end_time": "2020-01-01T13:30:00.000Z",
    "status": "confirmed",
    "event_type": "https://api.calendly.com/event_types/typeuuid456",
    "location": "https://meet.google.com/xyz-abc-def",
    "invitees_counter": {
      "total": 2,
      "confirmed": 1,
      "declined": 1
    },
    "questions_and_answers": [
      {
        "answer": "John Doe",
        "question": "Name"
      }
    ]
  }
}
```

#### Response Fields

The response wraps the event object in a `resource` key.

| Field | Type | Description |
|-------|------|-------------|
| `uri` | string | Canonical API URI of the scheduled event. |
| `uuid` | string | The unique identifier for the event. |
| `created_at` | string | ISO 8601 timestamp when the event was created. |
| `start_time` | string | ISO 8601 start time of the event. |
| `end_time` | string | ISO 8601 end time of the event. |
| `status` | string | Event status. Allowed values: `"confirmed"`, `"cancelled"` (British spelling, double 'l'). See [Event Status Enum](#event-status-enum). |
| `event_type` | string | API URI of the associated event type. |
| `location` | string | The location of the event (physical address or virtual meeting URL). |
| `invitees_counter` | [Invitees Counter](#invitees-counter-object) | Aggregate counts of invitees by status. |
| `questions_and_answers` | array of [Question And Answer](#question-and-answer-object) | Questions and answers associated with the event. |

### Error Responses

| Status | Description |
|--------|-------------|
| `401` | Invalid or missing access token. |
| `403` | Insufficient permissions to access this event. |
| `404` | Scheduled event not found. |
| `429` | Rate limit exceeded. |

---

## 2. List Scheduled Events

Retrieve a paginated list of scheduled events for a user. Supports filtering by status, time range, and invitee email.

| Property | Value |
|----------|-------|
| **Method** | `GET` |
| **Path** | `/scheduled_events` |
| **CLI Command** | `calendly events list [--status <active\|canceled>] [--min-start <iso8601>] [--max-start <iso8601>] [--count N] [--all] [--sort <field:dir>] [--json]` |

### Query Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `user` | string (URI) | **Yes** | -- | Full user URI. CLI auto-populates from cache. |
| `organization` | string (URI) | No | -- | Organization URI filter. |
| `count` | integer | No | 20 | Number of results to return per page. Range: 1-100. |
| `page_token` | string | No | -- | Token for cursor-based pagination. Obtained from a previous response's `pagination.next_page_token`. |
| `sort` | string | No | -- | Sort order. Values: `start_time:asc`, `start_time:desc`. |
| `status` | string | No | -- | Filter by event status: `active` or `canceled`. |
| `min_start_time` | string (ISO 8601) | No | -- | Lower bound on event start time (inclusive). |
| `max_start_time` | string (ISO 8601) | No | -- | Upper bound on event start time (inclusive). |
| `invitee_email` | string | No | -- | Filter events by invitee email address. |

### Request Headers

| Header | Type | Required | Default | Allowed Values |
|--------|------|----------|---------|----------------|
| `Authorization` | string | **Yes** | -- | `Bearer <token>` |
| `Content-Type` | string | No | `application/json` | `application/json` |

### Response `200 OK`

Standard paginated collection response: `{ "collection": [...], "pagination": {...} }`.

Collection items use the same schema as the `GET /scheduled_events/{uuid}` response fields (the inner `resource` object).

```json
{
  "collection": [
    {
      "uri": "https://api.calendly.com/scheduled_events/eventuuid123",
      "uuid": "eventuuid123",
      "created_at": "2020-01-01T12:00:00.000Z",
      "start_time": "2020-01-01T13:00:00.000Z",
      "end_time": "2020-01-01T13:30:00.000Z",
      "status": "confirmed",
      "event_type": "https://api.calendly.com/event_types/typeuuid456",
      "location": "https://meet.google.com/xyz-abc-def",
      "invitees_counter": {
        "total": 2,
        "confirmed": 1,
        "declined": 1
      },
      "questions_and_answers": []
    }
  ],
  "pagination": {
    "count": 1,
    "next_page": null,
    "previous_page": null,
    "next_page_token": null,
    "previous_page_token": null
  }
}
```

### Error Responses

| Status | Description |
|--------|-------------|
| `400` | Bad request (invalid query parameters). |
| `401` | Invalid or missing access token. |
| `403` | Insufficient permissions. |
| `429` | Rate limit exceeded. |

### CLI Command

```
calendly events list [--status <active|canceled>] [--min-start <iso8601>] [--max-start <iso8601>] [--count N] [--all] [--sort <field:dir>] [--json]
```

**Smart defaults:**
- `--min-start` defaults to `now` (current time).
- `--max-start` defaults to `now + 7 days`.
- User URI is auto-populated from cache.

**Shorthand flags:**
- `--today` -- sets min-start to start of today, max-start to end of today.
- `--tomorrow` -- sets min-start to start of tomorrow, max-start to end of tomorrow.
- `--this-week` -- sets min-start to now, max-start to end of current week (Sunday).

---

## 3. Create Scheduled Event

Create a new scheduled event with invitee details. Returns the event resource URI and a full invitee object.

| Property | Value |
|----------|-------|
| **Method** | `POST` |
| **Path** | `/scheduled_events` |
| **CLI Command** | `calendly events create --event-type <uri> --start-time <iso8601> --name <name> --email <email> --timezone <tz> [--location-kind <kind>] [--guest <email>...] [--qa <q:a>...]` |

### Request Headers

| Header | Type | Required | Default | Allowed Values |
|--------|------|----------|---------|----------------|
| `Authorization` | string | **Yes** | -- | `Bearer <token>` |
| `Content-Type` | string | **Yes** | `application/json` | `application/json` |

### Request Body (JSON)

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `event_type` | string | **Yes** | The URI of the event type to be scheduled. Example: `"https://api.calendly.com/event_types/AAAAAAAAAAAAAAAA"` |
| `start_time` | string | **Yes** | The start time of the event in ISO 8601 format. Example: `"2019-08-07T06:05:04.321123Z"` |
| `invitee` | [Invitee Request](#invitee-request-object) | **Yes** | Information about the invitee being scheduled. |
| `location` | [Location Request](#location-request-object) | Conditional | The meeting location details. Required if the event type specifies a location. |
| `questions_and_answers` | array of [Question And Answer](#question-and-answer-object) | No | Responses to custom questions configured on the event type. |
| `tracking` | [Tracking](#tracking-object) | No | UTM and Salesforce tracking parameters. |
| `event_guests` | array of string | No | Email addresses of additional guests to invite. Example: `["janedoe@calendly.com"]` |

#### Invitee Request Object

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `name` | string | **Yes** | Full name of the invitee. Example: `"John Smith"` |
| `first_name` | string | No | First name of the invitee. Example: `"John"` |
| `last_name` | string | No | Last name of the invitee. Example: `"Smith"` |
| `email` | string | **Yes** | Email address of the invitee. Example: `"test@example.com"` |
| `timezone` | string | **Yes** | IANA timezone of the invitee. Example: `"America/New_York"` |
| `text_reminder_number` | string/null | No | Phone number for SMS reminders. Example: `"+1 888-888-8888"` |

> **Note:** If `first_name` and `last_name` are omitted, the API may derive them from `name`. The CLI auto-splits `--name` into first/last by default. `[DEFERRED TO QA]` -- Verify whether POST /scheduled_events fails without first_name/last_name.

#### Location Request Object

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `kind` | string | **Yes** (if location provided) | The type of location. Allowed values: `"physical"`, `"virtual"`, `"phone_call"`, `"zoom_conference"`, `"google_hangouts"`, `"ask_invitee"`. |
| `location` | string | No | Specific location details (e.g., address or URL). Required for some `kind` values like `"physical"`. |

### Example Request

```json
{
  "event_type": "https://api.calendly.com/event_types/AAAAAAAAAAAAAAAA",
  "start_time": "2019-08-07T06:05:04.321123Z",
  "invitee": {
    "name": "John Smith",
    "first_name": "John",
    "last_name": "Smith",
    "email": "test@example.com",
    "timezone": "America/New_York",
    "text_reminder_number": "+1 888-888-8888"
  },
  "location": {
    "kind": "physical",
    "location": "123 Main St, Suite 100"
  },
  "questions_and_answers": [
    {
      "question": "Company Name",
      "answer": "Acme Corp",
      "position": 0
    }
  ],
  "tracking": {
    "utm_campaign": "spring_launch",
    "utm_source": "website",
    "utm_medium": "cta",
    "utm_content": "hero_banner",
    "utm_term": "scheduling",
    "salesforce_uuid": "sf-001-abc"
  },
  "event_guests": [
    "janedoe@calendly.com"
  ]
}
```

### Response `201 Created`

```json
{
  "resource": "https://api.calendly.com/scheduled_events/AAAAAAAAAAAAAAAA",
  "invitee": {
    "uri": "https://api.calendly.com/scheduled_events/AAAAAAAAAAAAAAAA/invitees/AAAAAAAAAAAAAAAA",
    "email": "test@example.com",
    "first_name": "John",
    "last_name": "Doe",
    "name": "John Doe",
    "status": "active",
    "questions_and_answers": [],
    "timezone": "America/New_York",
    "event": "https://api.calendly.com/scheduled_events/AAAAAAAAAAAAAAAA",
    "created_at": "2019-01-02T03:04:05.678123Z",
    "updated_at": "2019-08-07T06:05:04.321123Z",
    "tracking": {},
    "text_reminder_number": "+1 404-555-1234",
    "rescheduled": false,
    "old_invitee": null,
    "new_invitee": null,
    "cancel_url": "https://calendly.com/cancellations/AAAAAAAAAAAAAAAA",
    "reschedule_url": "https://calendly.com/reschedules/AAAAAAAAAAAAAAAA",
    "routing_form_submission": null,
    "cancellation": {},
    "payment": null,
    "no_show": null,
    "reconfirmation": null,
    "scheduling_method": "instant_book",
    "invitee_scheduled_by": null
  }
}
```

#### Top-Level Response Fields

| Field | Type | Description |
|-------|------|-------------|
| `resource` | string (URI) | Canonical reference (unique identifier) for the created scheduled event. |
| `invitee` | [Invitee Response](#invitee-response-object) | Full details about the invitee created for this event. |

> **IMPORTANT:** Unlike other Calendly API endpoints where `resource` wraps a full object, this endpoint returns `resource` as a plain URI string pointing to the created event. Do NOT attempt to parse `resource` as an object. Use `GET /scheduled_events/{uuid}` to retrieve the full event details. The implementation must define a distinct response struct for this endpoint (not reuse a generic `ResourceResponse<T>`).

#### Invitee Response Object

| Field | Type | Description |
|-------|------|-------------|
| `uri` | string | Canonical reference (unique identifier) for the invitee. |
| `email` | string | The invitee's email address. |
| `first_name` | string/null | The first name of the invitee. |
| `last_name` | string/null | The last name of the invitee. |
| `name` | string | The invitee's full name. |
| `status` | string | Invitee status. Allowed values: `"active"`, `"canceled"`. See [Invitee Status Enum](#invitee-status-enum) for the full 4-value superset. |
| `questions_and_answers` | array of [Question And Answer](#question-and-answer-object) | The invitee's responses to custom questions. |
| `timezone` | string/null | IANA timezone used when displaying time to the invitee. |
| `event` | string | API URI reference to the scheduled event. |
| `created_at` | string | ISO 8601 timestamp when the invitee was created. |
| `updated_at` | string | ISO 8601 timestamp when the invitee was last updated. |
| `tracking` | [Tracking](#tracking-object) | UTM and Salesforce tracking parameters associated with the invitee. |
| `text_reminder_number` | string/null | Phone number for SMS reminders. |
| `rescheduled` | boolean | Whether this invitee has rescheduled the event. |
| `old_invitee` | string/null | URI of the previous invitee instance before rescheduling. `null` if not rescheduled. |
| `new_invitee` | string/null | URI of the new invitee instance after rescheduling. `null` if not rescheduled. |
| `cancel_url` | string | URL the invitee can use to cancel the event. |
| `reschedule_url` | string | URL the invitee can use to reschedule the event. |
| `routing_form_submission` | string/null | URI reference to a routing form submission, if applicable. |
| `cancellation` | [Cancellation](#cancellation-object) | Data pertaining to the cancellation of the event or invitee. Empty object `{}` if not cancelled. |
| `payment` | object/null | Payment information for paid events. `null` if no payment required. |
| `no_show` | object/null | Data pertaining to an invitee no-show. `null` if not marked as no-show. |
| `reconfirmation` | object/null | Reconfirmation details. `null` if reconfirmation not required. |
| `scheduling_method` | string/null | The method used to schedule the event (e.g., `"instant_book"`). |
| `invitee_scheduled_by` | string/null | URI of the user who scheduled the event on behalf of the invitee. `null` if self-scheduled. |

### Error Responses

| Status | Description |
|--------|-------------|
| `400` | Bad request (malformed JSON or invalid field values). |
| `401` | Invalid or missing access token. |
| `403` | Insufficient permissions. |
| `404` | Event type not found. |
| `409` | Scheduling conflict (time slot no longer available). |
| `422` | Unprocessable entity (validation errors, e.g., missing required fields). |
| `429` | Rate limit exceeded. |

---

## 4. Cancel Scheduled Event

Cancel an existing scheduled event by its UUID.

| Property | Value |
|----------|-------|
| **Method** | `POST` |
| **Path** | `/scheduled_events/{uuid}/cancellation` |
| **CLI Command** | `calendly events cancel <uuid> [--reason <text>] [--yes]` |

### Path Parameters

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `uuid` | string | **Yes** | The UUID of the scheduled event to cancel. |

### Request Headers

| Header | Type | Required | Default | Allowed Values |
|--------|------|----------|---------|----------------|
| `Authorization` | string | **Yes** | -- | `Bearer <token>` |
| `Content-Type` | string | **Yes** | `application/json` | `application/json` |

### Request Body (JSON)

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `reason` | string | No | Reason for cancellation. |

### Example Request

```json
{
  "reason": "Schedule conflict -- need to reschedule."
}
```

### Response `201 Created`

```json
{
  "canceled_by": "host",
  "reason": "Schedule conflict -- need to reschedule."
}
```

#### Response Fields

| Field | Type | Description |
|-------|------|-------------|
| `canceled_by` | string | The party that initiated the cancellation (e.g., `"host"`, `"invitee"`). |
| `reason` | string/null | The reason provided for the cancellation. `null` if no reason was given. |

### Error Responses

| Status | Description |
|--------|-------------|
| `401` | Invalid or missing access token. |
| `403` | Insufficient permissions to cancel this event. |
| `404` | Scheduled event not found. |
| `409` | Event has already been cancelled. |
| `429` | Rate limit exceeded. |

### CLI Command

```
calendly events cancel <uuid> [--reason <text>] [--yes]
```

Requires confirmation prompt unless `--yes` is passed. When stdin is not a TTY, the prompt is automatically skipped.

---

## 5. Shared Schemas

### Question And Answer Object

Used in both request bodies and response payloads across Get Event and Create Scheduled Event endpoints.

| Field | Type | Required (Request) | Description |
|-------|------|--------------------|-------------|
| `question` | string | **Yes** | The question text. |
| `answer` | string | **Yes** | The invitee's answer to the question. |
| `position` | integer | **Conditional** | The display position/order of the question (0-indexed). Required in request bodies. May be absent in GET response payloads. |

### Invitees Counter Object

Returned in the Get Event response to provide aggregate invitee counts.

| Field | Type | Description |
|-------|------|-------------|
| `total` | integer | Total number of invitees for the event. |
| `confirmed` | integer | Number of invitees who confirmed attendance. |
| `declined` | integer | Number of invitees who declined the invitation. |

### Tracking Object

UTM and Salesforce tracking parameters. Used in both request bodies and response payloads.

> **Note:** The `tracking` field is always present in invitee responses. It is `{}` when no tracking was provided, NOT `null` or absent.

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `utm_campaign` | string | No | UTM campaign parameter. |
| `utm_source` | string | No | UTM source parameter. |
| `utm_medium` | string | No | UTM medium parameter. |
| `utm_content` | string | No | UTM content parameter. |
| `utm_term` | string | No | UTM term parameter. |
| `salesforce_uuid` | string | No | Salesforce UUID for CRM integration. |

### Cancellation Object

Provides data pertaining to the cancellation of an event or invitee. Present in the invitee response; empty object `{}` when no cancellation has occurred.

> **Note:** The `cancellation` field is always present in invitee responses. It is `{}` when no cancellation has occurred, NOT `null` or absent. Both sub-fields are absent when the cancellation object is empty.

| Field | Type | Description |
|-------|------|-------------|
| `canceled_by` | string | The party that initiated the cancellation (e.g., `"host"`, `"invitee"`). Absent when the cancellation object is empty `{}`. Implementation must treat this as optional during deserialization. |
| `reason` | string/null | The reason provided for the cancellation. Absent when the cancellation object is empty `{}`. |

### Location Request Object

Specifies the meeting location when creating a scheduled event.

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `kind` | string | **Yes** | The type of location. See [Location Kind Enum](#location-kind-enum) for allowed values. |
| `location` | string | Conditional | Specific location details. Required when `kind` is `"physical"`. |

### Location Kind Enum (Canonical Superset)

This is the harmonized superset of all known `location.kind` values across all Calendly API endpoints. Individual endpoints may accept/return only a subset. The CLI accepts any canonical value or alias via `--location-kind` and normalizes to the value expected by the target endpoint.

| Canonical Value | Description | Aliases |
|----------------|-------------|---------|
| `physical` | In-person meeting at a physical address. | -- |
| `ask_invitee` | Invitee chooses or provides the location. | -- |
| `custom` | Custom location with free-text. | -- |
| `zoom_conference` | Zoom video conference. | `zoom` |
| `google_conference` | Google Meet. | `google_hangouts` |
| `microsoft_teams_conference` | Microsoft Teams. | `microsoft_teams` |
| `outbound_call` | Host calls invitee. | `phone_call` (when direction unspecified) |
| `inbound_call` | Invitee calls host. | -- |
| `gotomeeting` | GoToMeeting. | -- |
| `webex` | Cisco Webex. | -- |
| `virtual` | Generic virtual meeting. `[DEFERRED TO QA]` -- Verify whether this is a real API value or a raw docs artifact. Remove if unconfirmed. | -- |
| _unknown_ | Forward-compatibility fallback for unrecognized values. | -- |

**Alias resolution rules:**
- `"zoom"` -> `"zoom_conference"`
- `"google_hangouts"` -> `"google_conference"`
- `"microsoft_teams"` -> `"microsoft_teams_conference"`
- `"phone_call"` -> `"outbound_call"` (default when direction unspecified)

Each spec retains its own enum table showing which subset that endpoint accepts/returns, but references this canonical superset.

### Event Status Enum

> **Note:** The event status uses `cancelled` (double 'l', British spelling). This differs from the invitee status which uses `canceled` (single 'l', American spelling). These are distinct string values on distinct resource types and must be handled separately. `[DEFERRED TO QA]` -- Verify the exact spelling against the live API. The implementation should accept both spellings defensively for event status (map both to the same enum variant), but send the documented spelling.

| Value | Description |
|-------|-------------|
| `confirmed` | The event is confirmed and scheduled. |
| `cancelled` | The event has been cancelled (British spelling, double 'l'). |

### Invitee Status Enum (Superset)

This is the single superset enum covering all invitee status values across all endpoints. Individual endpoints return only a subset of these values.

| Value | Description | Returned By |
|-------|-------------|-------------|
| `active` | The invitee is active and the booking is confirmed. | POST /scheduled_events, POST /invitees, GET /invitees list, GET /invitees/{uuid} |
| `canceled` | The invitee has cancelled their attendance (American spelling, single 'l'). | POST /scheduled_events, POST /invitees, GET /invitees list, GET /invitees/{uuid} |
| `accepted` | The invitee has accepted the invitation. | GET /invitees/{uuid} only |
| `declined` | The invitee has declined the invitation. | GET /invitees/{uuid} only |
| _unknown_ | Forward-compatibility fallback for unrecognized values. | -- |

> Implementation: Always deserialize using the superset. Use the `unknown` fallback for any unrecognized string value.

---

## 6. Error Responses

All events endpoints share the same error response formats.

### 400 Bad Request

Returned when the request body is malformed or contains invalid field values.

### 401 Unauthorized

```json
{
  "title": "Unauthenticated",
  "message": "The access token is invalid"
}
```

### 403 Forbidden

```json
{
  "title": "Permission Denied",
  "message": "You do not have permission to access this resource"
}
```

### 404 Not Found

```json
{
  "title": "Resource Not Found",
  "message": "The resource you are looking for does not exist"
}
```

### 409 Conflict

Returned when the requested time slot is no longer available for scheduling.

### 422 Unprocessable Entity

Returned when request validation fails (e.g., missing required fields, invalid URI format for `event_type`).

### 429 Too Many Requests

```http
HTTP/2 429 Too Many Requests
X-RateLimit-Limit: {your limit}
X-RateLimit-Remaining: 0
X-RateLimit-Reset: 60
```

---

## 7. Rate Limits

- Calendly enforces per-endpoint rate limits based on your plan.
- Rate limit information is returned via response headers on every request:

| Header | Type | Description |
|--------|------|-------------|
| `X-RateLimit-Limit` | integer | Maximum number of requests allowed in the current window. |
| `X-RateLimit-Remaining` | integer | Number of requests remaining in the current window. |
| `X-RateLimit-Reset` | integer | Number of seconds until the rate limit window resets. |

- When the limit is exceeded, a `429 Too Many Requests` response is returned.
- Implement exponential backoff using the `X-RateLimit-Reset` header value to determine retry timing.

---

## 8. Relationship: POST /scheduled_events vs POST /invitees

Both `POST /scheduled_events` and `POST /invitees` accept the **same request body** for booking a meeting. The difference lies in the response:

| Aspect | `POST /scheduled_events` | `POST /invitees` |
|--------|--------------------------|------------------|
| **Module** | Events (this module) | Invitees |
| **Request Body** | Identical | Identical |
| **Response Status** | `201 Created` | `200 OK` |
| **Response: Event Resource** | `resource` field (URI string) at top level, plus `invitee` wrapper object | No wrapper -- `event` field (URI string) is a top-level field in the flat response |
| **Response Shape** | Nested: `{ "resource": "<uri>", "invitee": {...} }` | Flat: all invitee fields at top level (no `resource` or `invitee` wrapper keys) |
| **Response: Invitee Fields** | Full invitee object with all fields (cancellation, payment, no_show, reconfirmation, scheduling_method, invitee_scheduled_by, cancel_url, reschedule_url, routing_form_submission) | Confirmed: event, status, timezone, cancel_url, reschedule_url, questions_and_answers, text_reminder_number, created_at, updated_at, rescheduled, old_invitee, new_invitee. Additional fields `[DEFERRED TO QA]`. |
| **Use Case** | When you need the event resource URI and full invitee details in a single call | When you only need basic booking confirmation and invitee data |

### Recommendation

Use `POST /scheduled_events` (this module) when your application needs:
- The canonical event URI for subsequent event lookups via `GET /scheduled_events/{event_uuid}`
- Full invitee metadata including cancellation, payment, no-show, and reconfirmation state
- The `scheduling_method` and `invitee_scheduled_by` fields for audit purposes

Use `POST /invitees` (invitees module) when your application only needs:
- Basic booking confirmation (status, timezone, cancel/reschedule URLs)
- Lightweight response payload
