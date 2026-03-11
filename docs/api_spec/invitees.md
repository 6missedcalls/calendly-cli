# Invitees Module API Specification

Base URL: `https://api.calendly.com`

---

## Table of Contents

- [Authentication](#authentication)
- [Rate Limits](#rate-limits)
- [Common Error Responses](#common-error-responses)
- [Workflow Dependencies](#workflow-dependencies)
- [1. Book a Meeting (Create Invitee)](#1-book-a-meeting-create-invitee)
- [2. List Event Invitees](#2-list-event-invitees)
- [3. Get Event Invitee](#3-get-event-invitee)

---

## Authentication

All requests to the Calendly API require a Bearer token in the `Authorization` header.

```
Authorization: Bearer {token}
```

Tokens are obtained via OAuth 2.0 authorization flow or from a Personal Access Token generated in the Calendly UI.

---

## Rate Limits

The Calendly API enforces rate limits on a per-token basis using a fixed-window strategy. When the rate limit is exceeded, the API responds with HTTP `429 Too Many Requests`.

**Rate Limit Response Headers:**

| Header | Type | Description |
|--------|------|-------------|
| `X-RateLimit-Limit` | integer | The maximum number of requests allowed in the current window |
| `X-RateLimit-Remaining` | integer | The number of requests remaining in the current window |
| `X-RateLimit-Reset` | integer | The number of seconds until the rate limit window resets |

**Example 429 Response:**

```http
HTTP/2 429 Too Many Requests
Date: Wed, 10 Jan 2024 12:00:00 GMT
Content-Type: application/json
X-RateLimit-Limit: 1000
X-RateLimit-Remaining: 0
X-RateLimit-Reset: 60
```

---

## Common Error Responses

The following error responses apply to all endpoints in this module.

### 400 Bad Request

Returned when the request is malformed or contains invalid parameters.

```json
{
  "title": "Invalid Argument",
  "message": "The request was invalid. Please verify the request parameters."
}
```

### 401 Unauthorized

Returned when the Bearer token is missing, expired, or invalid.

```json
{
  "title": "Unauthenticated",
  "message": "The access token is expired or invalid."
}
```

### 403 Forbidden

Returned when the authenticated user does not have permission to access the requested resource.

```json
{
  "title": "Permission Denied",
  "message": "You do not have permission to access this resource."
}
```

### 404 Not Found

Returned when the requested resource does not exist.

```json
{
  "title": "Resource Not Found",
  "message": "The requested resource was not found."
}
```

### 429 Too Many Requests

Returned when the API rate limit has been exceeded. See [Rate Limits](#rate-limits) for header details.

```json
{
  "title": "Too Many Requests",
  "message": "You have exceeded the rate limit. Please retry after the window resets."
}
```

### 500 Internal Server Error

Returned when an unexpected server-side error occurs.

```json
{
  "title": "Internal Server Error",
  "message": "An unexpected error occurred. Please try again later."
}
```

---

## Workflow Dependencies

The invitees module -- particularly the "book" command -- is the primary command designed for AI-agent workflows. Booking a meeting requires information from two prerequisite endpoints. The full workflow is:

### Step 1: Get the Event Type URI

Call `GET /event_types` (or use `calendly event-types list`) to retrieve the list of available event types. Identify the desired event type and note its `uri` field.

```
calendly event-types list
```

### Step 2: Get an Available Time Slot

Call `GET /event_type_available_times` (or use `calendly event-types available-times`) with the event type URI and a time range (max 7 days, must be in the future) to retrieve available start times.

```
calendly event-types available-times --event-type <event_type_uri> --start-time <iso8601> --end-time <iso8601>
```

### Step 3: Book the Meeting

Call `POST /invitees` (or use `calendly invitees book`) with the event type URI, a chosen `start_time` from the available times, and the invitee's details.

```
calendly invitees book --event-type <uri> --start-time <iso8601> --name <name> --email <email> --timezone <tz>
```

---

## 1. Book a Meeting (Create Invitee)

### Endpoint

```
POST /invitees
```

### Description

Creates an invitee and books a meeting on the host's calendar. This is the primary AI-agent endpoint for programmatic scheduling. It requires the event type URI (from `GET /event_types`), an available start time (from `GET /event_type_available_times`), and the invitee's contact details. The `location` object is required if the event type specifies one. Upon success, the response includes the scheduled event URI, cancellation and reschedule URLs, and the full invitee record.

### Authentication

- **Header:** `Authorization: Bearer <personal_access_token>`

### Parameters

#### Path Parameters

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| — | — | — | This endpoint has no path parameters |

#### Query Parameters

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| — | — | — | This endpoint has no query parameters |

#### Request Body

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `event_type` | string | Yes | The full URI of the event type to schedule (e.g., `https://api.calendly.com/event_types/AAAAAAAAAAAAAAAA`). Obtained from `GET /event_types`. |
| `start_time` | string | Yes | The start time of the event in ISO 8601 format (e.g., `2024-03-15T14:00:00.000000Z`). Must match an available slot from `GET /event_type_available_times`. |
| `invitee` | object | Yes | Details about the person being invited. See Invitee Object below. |
| `location` | object | Conditional | The meeting location details. Required if the event type has a location configured. See Location Object below. |
| `event_guests` | array\<string\> | No | Array of email addresses for additional event guests (e.g., `["guest1@example.com", "guest2@example.com"]`). |
| `questions_and_answers` | array\<object\> | No | Array of question and answer objects for custom questions configured on the event type. See Questions and Answers Object below. |
| `tracking` | object | No | UTM and Salesforce tracking parameters. See Tracking Object below. |

**Invitee Object:**

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `name` | string | Yes | Full name of the invitee |
| `first_name` | string | No | First name of the invitee |
| `last_name` | string | No | Last name of the invitee |
| `email` | string | Yes | Email address of the invitee |
| `timezone` | string | Yes | IANA timezone identifier for the invitee (e.g., `America/New_York`, `Europe/London`) |
| `text_reminder_number` | string | No | Phone number for SMS text reminders (e.g., `+1 888-888-8888`) |

**Location Object:**

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `kind` | string | Yes (if location provided) | The type of meeting location. See Location Kind enum below. |
| `location` | string | No | Specific location details (e.g., a physical address or custom meeting URL). Required for some `kind` values such as `physical`. |

**Location Kind Enum Values (this endpoint's subset):**

| Value | Description |
|-------|-------------|
| `"zoom_conference"` | Zoom video conference (auto-generated link) |
| `"google_hangouts"` | Google Meet conference |
| `"physical"` | In-person meeting at a physical address (requires `location` field) |
| `"ask_invitee"` | Ask the invitee to provide the location |
| `"phone_call"` | Phone call meeting |

> For the full harmonized superset of all known location kinds (including aliases), see the [Location Kind Enum in events.md](events.md#location-kind-enum-canonical-superset).

**Questions and Answers Object:**

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `question` | string | Yes | The question text as configured on the event type |
| `answer` | string | Yes | The answer provided by the invitee |
| `position` | integer | Yes | The ordinal position of the question (0-indexed) |

**Tracking Object:**

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `utm_campaign` | string | No | UTM campaign parameter |
| `utm_source` | string | No | UTM source parameter |
| `utm_medium` | string | No | UTM medium parameter |
| `utm_content` | string | No | UTM content parameter |
| `utm_term` | string | No | UTM term parameter |
| `salesforce_uuid` | string | No | Salesforce UUID for CRM tracking |

### Request Example

**cURL (minimal required fields):**

```bash
curl --request POST \
  --url https://api.calendly.com/invitees \
  --header 'Authorization: Bearer {token}' \
  --header 'Content-Type: application/json' \
  --data '{
    "event_type": "https://api.calendly.com/event_types/AAAAAAAAAAAAAAAA",
    "start_time": "2024-03-15T14:00:00.000000Z",
    "invitee": {
      "name": "Jane Smith",
      "email": "jane.smith@example.com",
      "timezone": "America/New_York"
    }
  }'
```

**cURL (all fields):**

```bash
curl --request POST \
  --url https://api.calendly.com/invitees \
  --header 'Authorization: Bearer {token}' \
  --header 'Content-Type: application/json' \
  --data '{
    "event_type": "https://api.calendly.com/event_types/AAAAAAAAAAAAAAAA",
    "start_time": "2024-03-15T14:00:00.000000Z",
    "invitee": {
      "name": "Jane Smith",
      "first_name": "Jane",
      "last_name": "Smith",
      "email": "jane.smith@example.com",
      "timezone": "America/New_York",
      "text_reminder_number": "+1 555-123-4567"
    },
    "location": {
      "kind": "zoom_conference"
    },
    "event_guests": [
      "colleague@example.com"
    ],
    "questions_and_answers": [
      {
        "question": "What is your company name?",
        "answer": "Acme Corp",
        "position": 0
      }
    ],
    "tracking": {
      "utm_source": "ai-agent",
      "utm_campaign": "outbound-scheduling",
      "salesforce_uuid": "sf-lead-001"
    }
  }'
```

### Response

#### Success Response (200 OK)

The response is a FLAT JSON object representing the invitee record. Unlike `POST /scheduled_events`, there are no `resource` or `invitee` wrapper keys.

> `[DEFERRED TO QA]` Verify against live API whether additional fields (`cancellation`, `payment`, `no_show`, `reconfirmation`, `scheduling_method`, `invitee_scheduled_by`) are returned by this endpoint. These fields are confirmed for `POST /scheduled_events` but unconfirmed for `POST /invitees`. Also verify whether the status code is 200 or 201 -- the implementation should accept both as success.

**Invitee Response Fields (top-level):**

| Field | Type | Nullable | Description |
|-------|------|----------|-------------|
| `uri` | string | No | Canonical API URI for the invitee (e.g., `https://api.calendly.com/scheduled_events/AAAA.../invitees/BBBB...`) |
| `email` | string | No | The invitee's email address |
| `first_name` | string | **Yes** | The invitee's first name. Null if not provided. `[DEFERRED TO QA]` -- Verify presence. |
| `last_name` | string | **Yes** | The invitee's last name. Null if not provided. `[DEFERRED TO QA]` -- Verify presence. |
| `name` | string | No | The invitee's full name |
| `status` | string | No | The invitee status. Values: `"active"`, `"canceled"`. See the [Invitee Status Enum in events.md](events.md#invitee-status-enum-superset) for the full 4-value superset. |
| `questions_and_answers` | array\<object\> | No | A collection of the invitee's responses to custom questions. Each object contains `question` (string), `answer` (string), and `position` (integer). |
| `timezone` | string | **Yes** | IANA timezone identifier used for display. Null if not provided. |
| `event` | string | No | Canonical API URI of the scheduled event |
| `created_at` | string | No | ISO 8601 timestamp of when the invitee was created |
| `updated_at` | string | No | ISO 8601 timestamp of the last update to the invitee record |
| `tracking` | object | No | The UTM and Salesforce tracking parameters associated with the invitee. Always present; `{}` if no tracking was provided, NOT `null` or absent. |
| `text_reminder_number` | string | **Yes** | The phone number for SMS reminders. Null if not provided. |
| `rescheduled` | boolean | No | Indicates whether this invitee has been rescheduled |
| `old_invitee` | string | **Yes** | Canonical URI of the previous invitee record if this booking was a reschedule. Null otherwise. |
| `new_invitee` | string | **Yes** | Canonical URI of the new invitee record if this booking has been rescheduled to a new time. Null otherwise. |
| `cancel_url` | string | No | Public URL the invitee can use to cancel the event (e.g., `https://calendly.com/cancellations/AAAA...`) |
| `reschedule_url` | string | No | Public URL the invitee can use to reschedule the event (e.g., `https://calendly.com/reschedules/AAAA...`) |

**Invitee Status Enum Values (this endpoint):**

| Value | Description |
|-------|-------------|
| `"active"` | The invitee is active and the event is confirmed |
| `"canceled"` | The invitee has cancelled the event (American spelling, single 'l') |

> For the full 4-value superset (including `accepted` and `declined`), see the [Invitee Status Enum in events.md](events.md#invitee-status-enum-superset).

### Response Example

```json
{
  "uri": "https://api.calendly.com/scheduled_events/AAAAAAAAAAAAAAAA/invitees/BBBBBBBBBBBBBBBB",
  "email": "jane.smith@example.com",
  "first_name": "Jane",
  "last_name": "Smith",
  "name": "Jane Smith",
  "status": "active",
  "questions_and_answers": [
    {
      "question": "What is your company name?",
      "answer": "Acme Corp",
      "position": 0
    }
  ],
  "timezone": "America/New_York",
  "event": "https://api.calendly.com/scheduled_events/AAAAAAAAAAAAAAAA",
  "created_at": "2024-03-15T13:55:00.000000Z",
  "updated_at": "2024-03-15T13:55:00.000000Z",
  "tracking": {
    "utm_source": "ai-agent",
    "utm_campaign": "outbound-scheduling"
  },
  "text_reminder_number": "+1 555-123-4567",
  "rescheduled": false,
  "old_invitee": null,
  "new_invitee": null,
  "cancel_url": "https://calendly.com/cancellations/AAAAAAAAAAAAAAAA",
  "reschedule_url": "https://calendly.com/reschedules/AAAAAAAAAAAAAAAA"
}
```

### Error Responses

In addition to the [Common Error Responses](#common-error-responses), this endpoint may return:

#### 400 Bad Request

Returned when required fields are missing, the `start_time` is not available, or the `event_type` URI is invalid.

```json
{
  "title": "Invalid Argument",
  "message": "The start_time is not an available time slot for this event type."
}
```

#### 409 Conflict

Returned when the requested time slot has already been booked by another invitee.

```json
{
  "title": "Conflict",
  "message": "The requested time slot is no longer available."
}
```

#### 422 Unprocessable Entity

Returned when the request body is syntactically valid but semantically incorrect (e.g., missing required invitee fields, invalid timezone, location required but not provided).

```json
{
  "title": "Unprocessable Entity",
  "message": "The invitee timezone is not a valid IANA timezone identifier."
}
```

### Pagination

This endpoint creates a single resource and does not support pagination.

### CLI Command Mapping

This endpoint is used by the following CLI command:

```
calendly invitees book --event-type <uri> --start-time <iso8601> --name <name> --email <email> --timezone <tz> [options...]
```

| Flag | Type | Required | Description |
|------|------|----------|-------------|
| `--event-type <uri>` | string | Yes | Full URI of the event type to book |
| `--start-time <iso8601>` | string | Yes | Start time in ISO 8601 format. Must be an available slot. |
| `--name <name>` | string | Yes | Full name of the invitee |
| `--email <email>` | string | Yes | Email address of the invitee |
| `--timezone <tz>` | string | Yes | IANA timezone identifier (e.g., `America/New_York`) |
| `--first-name <name>` | string | No | First name of the invitee |
| `--last-name <name>` | string | No | Last name of the invitee |
| `--text-reminder <phone>` | string | No | Phone number for SMS reminders (e.g., `+1 555-123-4567`) |
| `--location-kind <kind>` | string | No | Location type (e.g., `zoom_conference`, `physical`, `google_hangouts`) |
| `--location <value>` | string | No | Location details (e.g., physical address). Required when `--location-kind` is `physical`. |
| `--guests <emails>` | string | No | Comma-separated list of guest email addresses |
| `--qa <q>:<a>` | string (repeatable) | No | Question and answer pair in `question:answer` format. May be specified multiple times. |
| `--utm-source <value>` | string | No | UTM source tracking parameter |
| `--utm-campaign <value>` | string | No | UTM campaign tracking parameter |
| `--utm-medium <value>` | string | No | UTM medium tracking parameter |
| `--utm-content <value>` | string | No | UTM content tracking parameter |
| `--utm-term <value>` | string | No | UTM term tracking parameter |
| `--salesforce-uuid <value>` | string | No | Salesforce UUID for CRM tracking |

**Example (minimal):**

```
calendly invitees book \
  --event-type "https://api.calendly.com/event_types/AAAAAAAAAAAAAAAA" \
  --start-time "2024-03-15T14:00:00.000000Z" \
  --name "Jane Smith" \
  --email "jane.smith@example.com" \
  --timezone "America/New_York"
```

**Example (with location and guests):**

```
calendly invitees book \
  --event-type "https://api.calendly.com/event_types/AAAAAAAAAAAAAAAA" \
  --start-time "2024-03-15T14:00:00.000000Z" \
  --name "Jane Smith" \
  --email "jane.smith@example.com" \
  --timezone "America/New_York" \
  --location-kind "zoom_conference" \
  --guests "colleague@example.com,manager@example.com" \
  --qa "Company:Acme Corp" \
  --utm-source "ai-agent"
```

### Notes

1. **This is the primary AI-agent endpoint.** It enables programmatic meeting booking and is designed for use in automated scheduling workflows.
2. The `event_type` must be a full Calendly API URI (e.g., `https://api.calendly.com/event_types/AAAAAAAAAAAAAAAA`), not a bare UUID. Obtain this from `GET /event_types`.
3. The `start_time` must correspond to an available slot returned by `GET /event_type_available_times`. Attempting to book an unavailable time will result in an error.
4. The `location` object is conditionally required. If the event type has a location configured (check via `GET /event_types/{uuid}` -> `locations` array), the `location` object must be provided with a matching `kind`.
5. The `event_guests` field accepts a flat array of email strings, not objects. Each email receives a guest invitation to the event.
6. The `questions_and_answers` array should match the custom questions configured on the event type. Use `GET /event_types/{uuid}` -> `custom_questions` to discover available questions, their positions, and whether they are required.
7. The `cancel_url` and `reschedule_url` in the response are public URLs that do not require authentication. They can be shared with the invitee directly.
8. The `rescheduled`, `old_invitee`, and `new_invitee` fields form a linked list of invitee records across reschedules. On initial booking, `rescheduled` is `false` and both URI fields are `null`.
9. The `scheduling_method` field indicates how the event was booked. When booked via this API endpoint, it is typically `instant_book`.

---

## 2. List Event Invitees

### Endpoint

```
GET /scheduled_events/{uuid}/invitees
```

### Description

Retrieves a paginated list of invitees for a specific scheduled event. This endpoint supports filtering by invitee `status` and `email`, sorting, and cursor-based pagination using `count` and `page_token`. The event UUID is extracted from the trailing path segment of a scheduled event URI.

### Authentication

- **Header:** `Authorization: Bearer <personal_access_token>`

### Parameters

#### Path Parameters

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `uuid` | string | Yes | The unique identifier of the scheduled event. Extracted from the trailing path segment of a scheduled event URI (e.g., from `https://api.calendly.com/scheduled_events/AAAAAAAAAAAAAAAA`, the UUID is `AAAAAAAAAAAAAAAA`). |

#### Query Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `count` | integer | No | 20 | The number of rows to return. Must be between 1 and 100. |
| `page_token` | string | No | — | Token to retrieve the next page of results. Obtained from a previous response's `pagination.next_page_token` or `pagination.previous_page_token`. |
| `status` | string | No | — | Filters invitees by their status. See Invitee Status enum below. |
| `sort` | string | No | — | Specifies the order of the results (e.g., `created_at:asc`, `created_at:desc`). |
| `email` | string | No | — | Filters invitees by email address. Returns only invitees matching the specified email. |

**Invitee Status Enum Values (this endpoint's subset):**

| Value | Description |
|-------|-------------|
| `"active"` | The invitee is active and the event is confirmed |
| `"canceled"` | The invitee has cancelled the event (American spelling, single 'l') |

> For the full 4-value superset (including `accepted` and `declined`), see the [Invitee Status Enum in events.md](events.md#invitee-status-enum-superset).

### Request Example

```shell
curl --request GET \
  --url 'https://api.calendly.com/scheduled_events/AAAAAAAAAAAAAAAA/invitees?count=20&status=active' \
  --header 'Authorization: Bearer {token}' \
  --header 'Content-Type: application/json'
```

### Response

#### Success Response (200)

The response is a JSON object containing a `collection` array and a `pagination` object.

**Top-Level Response:**

| Field | Type | Description |
|-------|------|-------------|
| `collection` | array[Invitee] | A list of invitee objects for the specified event |
| `pagination` | Pagination | Contains information about the pagination of the results |

**Invitee Object:**

| Field | Type | Nullable | Description |
|-------|------|----------|-------------|
| `uri` | string | No | Canonical API URI for the invitee |
| `email` | string | No | The invitee's email address |
| `first_name` | string | **Yes** | The invitee's first name. Null if not provided. |
| `last_name` | string | **Yes** | The invitee's last name. Null if not provided. |
| `name` | string | No | The invitee's full name |
| `status` | string | No | The invitee status (`active` or `canceled`). See the [Invitee Status Enum in events.md](events.md#invitee-status-enum-superset) for the full 4-value superset. |
| `questions_and_answers` | array\<object\> | No | A collection of the invitee's responses to custom questions |
| `timezone` | string | **Yes** | IANA timezone identifier. Null if not provided. |
| `event` | string | No | Canonical API URI of the scheduled event |
| `created_at` | string | No | ISO 8601 timestamp of when the invitee was created |
| `updated_at` | string | No | ISO 8601 timestamp of the last update |
| `tracking` | object | No | UTM and Salesforce tracking parameters. Always present; `{}` when no tracking was provided, NOT `null` or absent. |
| `text_reminder_number` | string | **Yes** | Phone number for SMS reminders. Null if not provided. |
| `rescheduled` | boolean | No | Indicates whether this invitee has been rescheduled |
| `old_invitee` | string | **Yes** | URI of the previous invitee record if rescheduled. Null otherwise. |
| `new_invitee` | string | **Yes** | URI of the new invitee record if rescheduled. Null otherwise. |
| `cancel_url` | string | No | Public URL to cancel the event |
| `reschedule_url` | string | No | Public URL to reschedule the event |
| `routing_form_submission` | string | **Yes** | Reference to a routing form submission. Null if not applicable. |
| `cancellation` | object | No | Cancellation data. Always present; `{}` when no cancellation has occurred, NOT `null` or absent. |
| `payment` | object | **Yes** | Payment information. Null if not a paid event. |
| `no_show` | object | **Yes** | No-show data. Null if not marked. |
| `reconfirmation` | object | **Yes** | Reconfirmation details. Null if not configured. |
| `scheduling_method` | string | **Yes** | The method used to schedule (e.g., `instant_book`). |
| `invitee_scheduled_by` | string | **Yes** | URI of the user who scheduled the event. Null if self-scheduled. |

**Pagination Object:**

| Field | Type | Description |
|-------|------|-------------|
| `count` | integer | The number of rows returned in this response |
| `next_page` | string or null | Full URI to return the next page of results. Null if there is no next page. |
| `previous_page` | string or null | Full URI to return the previous page of results. Null if there is no previous page. |
| `next_page_token` | string or null | Token to return the next page of results. Pass this as the `page_token` query parameter. Null if there is no next page. |
| `previous_page_token` | string or null | Token to return the previous page of results. Pass this as the `page_token` query parameter. Null if there is no previous page. |

### Response Example

```json
{
  "collection": [
    {
      "uri": "https://api.calendly.com/scheduled_events/AAAAAAAAAAAAAAAA/invitees/BBBBBBBBBBBBBBBB",
      "email": "jane.smith@example.com",
      "first_name": "Jane",
      "last_name": "Smith",
      "name": "Jane Smith",
      "status": "active",
      "questions_and_answers": [
        {
          "question": "What is your company name?",
          "answer": "Acme Corp",
          "position": 0
        }
      ],
      "timezone": "America/New_York",
      "event": "https://api.calendly.com/scheduled_events/AAAAAAAAAAAAAAAA",
      "created_at": "2024-03-15T13:55:00.000000Z",
      "updated_at": "2024-03-15T13:55:00.000000Z",
      "tracking": {},
      "text_reminder_number": null,
      "rescheduled": false,
      "old_invitee": null,
      "new_invitee": null,
      "cancel_url": "https://calendly.com/cancellations/BBBBBBBBBBBBBBBB",
      "reschedule_url": "https://calendly.com/reschedules/BBBBBBBBBBBBBBBB",
      "routing_form_submission": null,
      "cancellation": {},
      "payment": null,
      "no_show": null,
      "reconfirmation": null,
      "scheduling_method": "instant_book",
      "invitee_scheduled_by": null
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

In addition to the [Common Error Responses](#common-error-responses), this endpoint may return:

#### 404 Not Found

Returned when the event UUID does not match any scheduled event.

```json
{
  "title": "Resource Not Found",
  "message": "The scheduled event could not be found."
}
```

### Pagination

Cursor-based. Use the `page_token` query parameter with a token from `pagination.next_page_token` or `pagination.previous_page_token` to navigate between pages. The `count` parameter controls page size (max 100, default 20). To fetch all results, continue requesting the next page until `pagination.next_page_token` is null.

### CLI Command Mapping

This endpoint is used by the following CLI command:

```
calendly invitees list <event_uuid> [--count N] [--all] [--status <s>] [--email <e>] [--sort <field>]
```

| Flag | Type | Required | Description |
|------|------|----------|-------------|
| `<event_uuid>` | positional | Yes | The UUID of the scheduled event (trailing segment of the event URI) |
| `--count N` | integer | No | Number of results per page (1-100, default 20) |
| `--all` | flag | No | Automatically paginate through all results and return the complete list |
| `--status <s>` | string | No | Filter by invitee status (`active`, `canceled`) |
| `--email <e>` | string | No | Filter by invitee email address |
| `--sort <field>` | string | No | Sort order (e.g., `created_at:asc`, `created_at:desc`) |

**Example:**

```
calendly invitees list AAAAAAAAAAAAAAAA --status active --count 50
```

**Example (fetch all invitees):**

```
calendly invitees list AAAAAAAAAAAAAAAA --all
```

The `--all` flag handles cursor-based pagination internally by following `next_page_token` until all pages have been fetched, then merges all `collection` arrays into a single output.

### Notes

1. The event UUID is the trailing path segment of a scheduled event URI. For example, if the event URI is `https://api.calendly.com/scheduled_events/AAAAAAAAAAAAAAAA`, the UUID to pass is `AAAAAAAAAAAAAAAA`.
2. The `uri`, `event`, `old_invitee`, `new_invitee`, and `invitee_scheduled_by` fields are full Calendly API URIs, not bare UUIDs. Implementations should parse the UUID from the trailing path segment when needed.
3. For events with many invitees (e.g., group events), use pagination or the `--all` flag to retrieve the complete list.
4. The `email` query parameter performs an exact match filter, not a partial search.
5. The `sort` parameter accepts a field name and direction separated by a colon (e.g., `created_at:asc` or `created_at:desc`).

---

## 3. Get Event Invitee

> **This endpoint returns a distinct resource type (InviteRecord) that differs significantly from the Invitee objects returned by the Book and List endpoints.** It uses the `/invites/` path prefix and returns a slim record with event timing and an invitation secret. Implementation must define two separate structs/types: `Invitee` (full object from Book/List, 22+ fields) and `InviteRecord` (slim object from this endpoint, 8 fields).

### Endpoint

```
GET /invitees/{invitee_uuid}
```

### Description

Retrieves the details of a specific event invitee by their UUID. This endpoint returns the invitee's status, associated event, scheduled time range, and an invitation secret token.

### Authentication

- **Header:** `Authorization: Bearer <personal_access_token>`

### Parameters

#### Path Parameters

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `invitee_uuid` | string | Yes | The unique identifier of the invitee. Extracted from the trailing path segment of an invitee URI. |

#### Query Parameters

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| — | — | — | This endpoint has no query parameters |

### Request Body

This endpoint does not accept a request body.

### Request Example

```bash
curl --request GET \
  --url https://api.calendly.com/invitees/BBBBBBBBBBBBBBBB \
  --header 'Authorization: Bearer {token}' \
  --header 'Content-Type: application/json'
```

### Response

#### Success Response (200) -- InviteRecord

| Field | Type | Nullable | Description |
|-------|------|----------|-------------|
| `uri` | string | No | Canonical API URI for the invite record (e.g., `https://api.calendly.com/invites/BBBBBBBBBBBBBBBB`). Note: uses `/invites/` path prefix, not `/invitees/`. |
| `invitee_uuid` | string | No | The unique identifier for the invitee |
| `status` | string | No | The status of the invitee. This endpoint returns all 4 values of the InviteeStatus superset. |
| `created_at` | string | No | ISO 8601 timestamp of when the invitee was created |
| `start_time` | string | No | ISO 8601 timestamp of the event start time |
| `end_time` | string | No | ISO 8601 timestamp of the event end time |
| `event` | string | No | Canonical API URI of the associated scheduled event |
| `invitation_secret` | string | No | A secret token for the invitation. Can be used for programmatic access to cancel/reschedule URLs. |

**InviteeStatus Enum Values (all 4 values returned by this endpoint):**

| Value | Description |
|-------|-------------|
| `"active"` | The invitee is active and the event is confirmed |
| `"canceled"` | The invitee has cancelled the event (American spelling, single 'l') |
| `"accepted"` | The invitee has accepted the invitation |
| `"declined"` | The invitee has declined the invitation |

> See the [Invitee Status Enum in events.md](events.md#invitee-status-enum-superset) for the canonical superset definition. The `accepted` and `declined` values are only returned by this endpoint.

### Response Example

```json
{
  "uri": "https://api.calendly.com/invites/BBBBBBBBBBBBBBBB",
  "invitee_uuid": "BBBBBBBBBBBBBBBB",
  "status": "accepted",
  "created_at": "2024-03-15T13:55:00.000000Z",
  "start_time": "2024-03-15T14:00:00.000000Z",
  "end_time": "2024-03-15T14:30:00.000000Z",
  "event": "https://api.calendly.com/scheduled_events/AAAAAAAAAAAAAAAA",
  "invitation_secret": "secret_abc123def456"
}
```

### Error Responses

In addition to the [Common Error Responses](#common-error-responses), this endpoint may return:

#### 404 Not Found

Returned when the invitee UUID does not match any existing invitee.

```json
{
  "title": "Resource Not Found",
  "message": "The invitee could not be found."
}
```

### Pagination

This endpoint returns a single resource and does not support pagination.

### CLI Command Mapping

This endpoint is used by the following CLI command:

```
calendly invitees show <invitee_uuid>
```

| Flag | Type | Required | Description |
|------|------|----------|-------------|
| `<invitee_uuid>` | positional | Yes | The UUID of the invitee to retrieve |

**Example:**

```
calendly invitees show BBBBBBBBBBBBBBBB
```

### Notes

1. This endpoint uses a different URI pattern than the list endpoint. The list endpoint nests invitees under `/scheduled_events/{uuid}/invitees`, while this endpoint uses `/invitees/{invitee_uuid}` directly.
2. The `uri` in the response uses the `/invites/` path prefix (e.g., `https://api.calendly.com/invites/BBBB...`), which differs from the nested URI format used in the list and book endpoints (e.g., `https://api.calendly.com/scheduled_events/AAAA.../invitees/BBBB...`). Both refer to the same underlying invitee record.
3. The `invitation_secret` is a sensitive token. It should not be logged or exposed to end users unless necessary for programmatic cancel/reschedule workflows.
4. Unlike the list endpoint response, this endpoint returns `start_time` and `end_time` directly on the invitee object, providing the event's time range without needing a separate call to `GET /scheduled_events/{uuid}`.
5. The `event` field is a full Calendly API URI. Parse the trailing path segment to extract the event UUID if needed for subsequent API calls (e.g., listing all invitees for that event).
