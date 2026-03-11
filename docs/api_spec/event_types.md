# Calendly Event Types API - Complete Endpoint Specification

> **Base URL:** `https://api.calendly.com`
>
> **Authentication:** All endpoints require:
> - `Authorization: Bearer <personal_access_token>` header
>
> **Content-Type:** `application/json`
>
> **Rate Limiting:** Calendly uses a fixed-window rate limiting strategy. When limits are exceeded, the API returns `429 Too Many Requests` with the following headers:
> - `X-RateLimit-Limit` -- your plan's request limit
> - `X-RateLimit-Remaining` -- requests remaining in the current window
> - `X-RateLimit-Reset` -- seconds until the rate limit window resets

---

## Table of Contents

1. [GET /event_types](#1-get-event_types) -- List event types
2. [GET /event_types/{uuid}](#2-get-event_typesuuid) -- Get event type details
3. [GET /event_type_available_times](#3-get-event_type_available_times) -- Get available time slots
4. [Common Error Responses](#common-error-responses)
5. [Enums](#enums)
6. [Pagination](#pagination)
7. [CLI Reference](#cli-reference)

---

## 1. GET /event_types

### Description

Retrieves a paginated collection of event types for a given user. The response includes each event type's `scheduling_url` for booking and `pooling_type` to understand the event's scheduling method (e.g., round robin, collective, or solo). Use the `active` filter to retrieve only enabled or disabled event types.

### Path Parameters

None.

### Query Parameters

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `user` | string | Yes | The URI of the user whose event types to retrieve. Format: `https://api.calendly.com/users/<user_uuid>` |
| `active` | boolean | No | Filter by active status. `true` returns only active event types; `false` returns only inactive. Omit to return all. |
| `count` | integer | No | Number of results to return per page. Must be between 1 and 100. Default: 20. |
| `page_token` | string | No | Token for cursor-based pagination. Obtained from `pagination.next_page_token` in a previous response. |
| `sort` | string | No | Sort order for results. Format: `<field>:<direction>`. Example: `name:asc`, `name:desc`. [UNDOCUMENTED - verify against live API for full list of sortable fields] |
| `organization` | string | No | The URI of the organization. Required when listing event types across an organization rather than a single user. Format: `https://api.calendly.com/organizations/<org_uuid>` [UNDOCUMENTED - verify against live API] |

### Request Body

None.

### Request Example

```bash
curl --request GET \
  --url 'https://api.calendly.com/event_types?user=https://api.calendly.com/users/AAAAAAAAAAAAAAAA&active=true&count=20' \
  --header 'Authorization: Bearer YOUR_ACCESS_TOKEN' \
  --header 'Content-Type: application/json'
```

### Response Body (200 OK)

```json
{
  "collection": [
    {
      "uri": "https://api.calendly.com/event_types/88323028-5ef5-448b-9776-bc0c71b062a4",
      "name": "15 Minute Meeting",
      "active": true,
      "booking_method": null,
      "slug": "15min",
      "scheduling_url": "https://calendly.com/acmesales/15min",
      "duration": 15,
      "kind": "solo",
      "pooling_type": null,
      "type": "EventType",
      "color": "#fff200",
      "created_at": "2019-08-07T06:05:04.321123Z",
      "updated_at": "2019-08-07T06:05:04.321123Z",
      "internal_note": null,
      "description_plain": "A quick 15-minute chat.",
      "description_html": "<p>A quick 15-minute chat.</p>",
      "profile": {
        "type": "User",
        "name": "Example User",
        "owner": "https://api.calendly.com/users/BBBBBBBBBBBBBBBB"
      },
      "secret": false,
      "deleted_at": null,
      "admin_managed": false,
      "custom_questions": []
    }
  ],
  "pagination": {
    "count": 20,
    "next_page": "https://api.calendly.com/event_types?count=20&page_token=sNjq4TvMDfUHEl7zHRR0k0E1PCEJWvdi",
    "previous_page": "https://api.calendly.com/event_types?count=20&page_token=VJs2rfDYeY8ahZpq0QI1O114LJkNjd7H",
    "next_page_token": "sNjq4TvMDfUHEl7zHRR0k0E1PCEJWvdi",
    "previous_page_token": "VJs2rfDYeY8ahZpq0QI1O114LJkNjd7H"
  }
}
```

### Response Fields

#### `collection[]` -- Event Type Object (summary)

| Field | Type | Description |
|-------|------|-------------|
| `uri` | string | Canonical reference (unique identifier) for the event type. Format: `https://api.calendly.com/event_types/<uuid>` |
| `name` | string | The name of the event type (e.g., "15 Minute Meeting") |
| `active` | boolean | Whether the event type is currently active and bookable |
| `booking_method` | string \| null | The booking method. Known values: `"poll"`, `null` |
| `slug` | string | The URL-friendly slug for the event type |
| `scheduling_url` | string | The public URL where invitees can book this event type |
| `duration` | integer | Duration of the event in minutes |
| `kind` | string | The kind of event. Known values: `"solo"`, `"group"` [UNDOCUMENTED - verify full enum against live API] |
| `pooling_type` | string \| null | The pooling/routing type for team events. Values: `"round_robin"`, `"collective"`, `null` |
| `type` | string | Resource type identifier. Always `"EventType"` |
| `color` | string | Hex color code associated with the event type (e.g., `"#fff200"`) |
| `created_at` | string | ISO 8601 timestamp when the event type was created |
| `updated_at` | string | ISO 8601 timestamp when the event type was last updated |
| `internal_note` | string \| null | Private note visible only to the event host |
| `description_plain` | string \| null | Plain text description of the event type. May be `null` for event types where the user has not set a description. |
| `description_html` | string \| null | HTML-formatted description of the event type. May be `null` for event types where the user has not set a description. |
| `profile` | object | Profile information for the user who owns this event type |
| `profile.type` | string | Profile type. Known values: `"User"`, `"Team"` [UNDOCUMENTED - verify against live API] |
| `profile.name` | string | Display name of the profile owner |
| `profile.owner` | string | URI of the user or organization that owns this event type |
| `secret` | boolean | Whether the event type is secret (unlisted, not shown on scheduling page) |
| `deleted_at` | string \| null | ISO 8601 timestamp when the event type was deleted, or `null` if not deleted |
| `admin_managed` | boolean | Whether the event type is managed by an organization admin |
| `custom_questions` | array | List of custom questions configured for this event type (see full schema in endpoint 2) |

#### `pagination` -- Pagination Object

| Field | Type | Description |
|-------|------|-------------|
| `count` | integer | Number of results returned in this page |
| `next_page` | string \| null | Full URL to retrieve the next page of results, or `null` if no more pages |
| `previous_page` | string \| null | Full URL to retrieve the previous page of results, or `null` if on first page |
| `next_page_token` | string \| null | Token to pass as `page_token` query parameter for the next page |
| `previous_page_token` | string \| null | Token to pass as `page_token` query parameter for the previous page |

### Error Responses

See [Common Error Responses](#common-error-responses).

### Notes

- The `user` parameter is **required** and must be the full URI (not just the UUID).
- The list endpoint returns a subset of fields compared to the detail endpoint. Use `GET /event_types/{uuid}` for the full event type object including `locations`, `custom_questions` details, and `duration_options`.
- Deleted event types are **not** returned by default. [UNDOCUMENTED - verify filtering behavior for deleted event types against live API]
- Maximum `count` is 100. Requesting more than 100 will be clamped or return an error.

---

## 2. GET /event_types/{uuid}

### Description

Retrieves the full details of a specific event type by its UUID. The response includes all configuration details: locations, custom questions, duration options, profile information, and scheduling settings.

### Path Parameters

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `uuid` | string | Yes | The unique identifier of the event type. Extract from the event type URI: `https://api.calendly.com/event_types/<uuid>` |

### Query Parameters

None.

### Request Body

None.

### Request Example

```bash
curl --request GET \
  --url https://api.calendly.com/event_types/AAAAAAAAAAAAAAAA \
  --header 'Authorization: Bearer YOUR_ACCESS_TOKEN' \
  --header 'Content-Type: application/json'
```

### Response Body (200 OK)

```json
{
  "resource": {
    "uri": "https://api.calendly.com/event_types/AAAAAAAAAAAAAAAA",
    "type": "EventType",
    "name": "15 Minute Meeting",
    "description_plain": "15 Minute Meeting",
    "description_html": "<p>15 Minute Meeting</p>",
    "duration": 15,
    "color": "#fff200",
    "kind": "solo",
    "slug": "acmesales",
    "scheduling_url": "https://calendly.com/acmesales",
    "secret": false,
    "is_paid": false,
    "pooling_type": null,
    "booking_method": null,
    "active": true,
    "admin_managed": false,
    "internal_note": null,
    "created_at": "2019-08-07T06:05:04.321123Z",
    "updated_at": "2019-08-07T06:05:04.321123Z",
    "deleted_at": null,
    "duration_options": [15, 30, 60],
    "locations": [
      {
        "kind": "ask_invitee",
        "location": null,
        "additional_info": null,
        "phone_number": null
      }
    ],
    "custom_questions": [
      {
        "name": "Please share anything that will help prepare for our meeting.",
        "type": "text",
        "position": 0,
        "enabled": true,
        "required": false,
        "answer_choices": [],
        "include_other": false
      }
    ],
    "profile": {
      "type": "User",
      "name": "Example User",
      "owner": "https://api.calendly.com/users/BBBBBBBBBBBBBBBB"
    }
  }
}
```

### Response Fields

#### `resource` -- Event Type Object (full)

| Field | Type | Description |
|-------|------|-------------|
| `uri` | string | Canonical reference (unique identifier) for the event type. Format: `https://api.calendly.com/event_types/<uuid>` |
| `type` | string | Resource type identifier. Always `"EventType"` |
| `name` | string | The display name of the event type (e.g., "15 Minute Meeting") |
| `description_plain` | string \| null | Plain text description of the event type. May be `null` for event types where the user has not set a description. |
| `description_html` | string \| null | HTML-formatted description of the event type. May be `null` for event types where the user has not set a description. |
| `duration` | integer | Default duration of the event in minutes |
| `color` | string | Hex color code associated with the event type (e.g., `"#fff200"`) |
| `kind` | string | The kind of event type. Known values: `"solo"`, `"group"` [UNDOCUMENTED - verify full enum against live API] |
| `slug` | string | The URL-friendly slug used in the scheduling URL |
| `scheduling_url` | string | The public URL where invitees can book this event type |
| `secret` | boolean | Whether the event type is secret (unlisted, accessible only via direct link) |
| `is_paid` | boolean | Whether the event type requires payment from the invitee |
| `pooling_type` | string \| null | The pooling/routing type for team events. Values: `"round_robin"`, `"collective"`, `null` (solo) |
| `booking_method` | string \| null | The booking method. Known values: `"poll"`, `null` [UNDOCUMENTED - verify full enum against live API] |
| `active` | boolean | Whether the event type is currently active and accepting bookings |
| `admin_managed` | boolean | Whether the event type is managed by an organization admin |
| `internal_note` | string \| null | Private note visible only to the event host, not shown to invitees |
| `created_at` | string | ISO 8601 timestamp when the event type was created |
| `updated_at` | string | ISO 8601 timestamp when the event type was last updated |
| `deleted_at` | string \| null | ISO 8601 timestamp when the event type was deleted, or `null` if not deleted |
| `duration_options` | array\<integer\> | List of selectable duration options in minutes (e.g., `[15, 30, 60]`). Empty array if only the default duration is available. [UNDOCUMENTED - verify empty vs. null behavior against live API] |
| `locations` | array\<object\> | List of configured meeting location options. See `locations[]` schema below. |
| `custom_questions` | array\<object\> | List of custom questions presented to invitees during booking. See `custom_questions[]` schema below. |
| `profile` | object | Profile information for the event type owner. See `profile` schema below. |

#### `resource.locations[]` -- Location Object

| Field | Type | Description |
|-------|------|-------------|
| `kind` | string | The type of location. Known values: `"ask_invitee"`, `"zoom"`, `"google_hangouts"`, `"physical"`, `"phone_call"`, `"gotomeeting"`, `"microsoft_teams"`, `"webex"`, `"custom"` [UNDOCUMENTED - verify full enum against live API] |
| `location` | string \| null | The specific location details. For virtual meetings this may be a URL; for physical meetings an address string. `null` when `kind` is `"ask_invitee"`. |
| `additional_info` | string \| null | Additional information or instructions about the location |
| `phone_number` | string \| null | Phone number for phone-based locations. `null` for non-phone location types. |

#### `resource.custom_questions[]` -- Custom Question Object

| Field | Type | Description |
|-------|------|-------------|
| `name` | string | The question text displayed to the invitee |
| `type` | string | The input type. Known values: `"text"`, `"dropdown"`, `"radio"`, `"checkbox"`, `"phone_number"` [UNDOCUMENTED - verify full enum against live API] |
| `position` | integer | The display order of the question (0-indexed) |
| `enabled` | boolean | Whether the question is currently enabled and shown to invitees |
| `required` | boolean | Whether the invitee must answer this question to complete booking |
| `answer_choices` | array\<string\> | List of predefined answer choices for `"dropdown"`, `"radio"`, and `"checkbox"` question types. Empty array for free-text types. |
| `include_other` | boolean | Whether an "Other" free-text option is shown in addition to `answer_choices` |

#### `resource.profile` -- Profile Object

| Field | Type | Description |
|-------|------|-------------|
| `type` | string | The type of profile. Known values: `"User"`, `"Team"` [UNDOCUMENTED - verify against live API] |
| `name` | string | The display name of the profile owner |
| `owner` | string | The URI of the user or organization that owns this event type. Format: `https://api.calendly.com/users/<uuid>` or `https://api.calendly.com/organizations/<uuid>` |

### Error Responses

See [Common Error Responses](#common-error-responses).

### Notes

- The response wraps the event type object in a `resource` key (not `collection`).
- The `locations` array may contain multiple entries when the event type offers location choice to the invitee.
- Custom questions with `enabled: false` are still returned in the response but are not shown to invitees during booking.
- The `duration_options` array is only populated when the event type allows invitees to choose from multiple durations. When only the default duration is available, this may be an empty array or contain only the single default value.

---

## 3. GET /event_type_available_times

### Description

Retrieves a collection of available time slots for a specific event type within a given time range. This endpoint is designed for building custom scheduling interfaces or embedding availability information in external applications. The time range must be in the future and cannot exceed 7 days.

### Path Parameters

None.

### Query Parameters

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `event_type` | string | Yes | The URI of the event type. Format: `https://api.calendly.com/event_types/<uuid>` |
| `start_time` | string | Yes | The start of the time range. Must be in the future. ISO 8601 format (e.g., `2024-02-16T00:00:00.000000Z`). |
| `end_time` | string | Yes | The end of the time range. Must be in the future and cannot be more than 7 days after `start_time`. ISO 8601 format (e.g., `2024-02-20T00:00:00.000000Z`). |

### Request Body

None.

### Request Example

```bash
curl --request GET \
  --url 'https://api.calendly.com/event_type_available_times?event_type=https://api.calendly.com/event_types/AAAAAAAAAAAAAAAA&start_time=2024-02-16T00:00:00.000000Z&end_time=2024-02-20T00:00:00.000000Z' \
  --header 'Authorization: Bearer YOUR_ACCESS_TOKEN' \
  --header 'Content-Type: application/json'
```

### Response Body (200 OK)

```json
{
  "collection": [
    {
      "status": "available",
      "start_time": "2024-02-16T09:00:00.000000Z",
      "invitees_remaining": 1
    },
    {
      "status": "available",
      "start_time": "2024-02-16T09:30:00.000000Z",
      "invitees_remaining": 1
    },
    {
      "status": "available",
      "start_time": "2024-02-16T10:00:00.000000Z",
      "invitees_remaining": 3
    }
  ]
}
```

### Response Fields

#### `collection[]` -- Available Time Slot Object

| Field | Type | Description |
|-------|------|-------------|
| `status` | string | The availability status of the time slot. Known value: `"available"` |
| `start_time` | string | The start time of the available slot in ISO 8601 format |
| `invitees_remaining` | integer | The number of invitee spots remaining for this time slot. For solo events this is typically `1`. For group events this reflects remaining capacity. |

### Error Responses

See [Common Error Responses](#common-error-responses).

Additionally, this endpoint may return:

#### 400 Bad Request -- Time Range Exceeds Maximum

Returned when `end_time` is more than 7 days after `start_time`.

```json
{
  "title": "Invalid Argument",
  "message": "The end_time must be within 7 days of start_time."
}
```

#### 400 Bad Request -- Start Time in the Past

Returned when `start_time` is not in the future.

```json
{
  "title": "Invalid Argument",
  "message": "The start_time must be in the future."
}
```

### Notes

- This endpoint does **not** support pagination. All available time slots within the requested range are returned in a single response.
- The maximum time range is **7 days**. To query availability beyond 7 days, make multiple sequential requests with non-overlapping windows.
- All timestamps in both the request and response use UTC (ISO 8601 format).
- The `invitees_remaining` field is particularly useful for group events where multiple invitees can book the same time slot.
- Time slots are returned in chronological order.
- [UNDOCUMENTED - verify whether the response includes a `pagination` object (always null/empty) or omits it entirely]

---

## Common Error Responses

### 401 Unauthorized

Returned when the `Authorization` header is missing, malformed, or the token is invalid/expired.

```json
{
  "title": "Unauthenticated",
  "message": "The access token is expired"
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

Returned when the requested event type UUID does not exist.

```json
{
  "title": "Resource Not Found",
  "message": "The resource you requested does not exist."
}
```

### 429 Too Many Requests

Returned when the rate limit has been exceeded. Inspect the response headers to determine when to retry.

```http
HTTP/2 429 Too Many Requests
Content-Type: application/json
X-RateLimit-Limit: {your limit}
X-RateLimit-Remaining: 0
X-RateLimit-Reset: 60
```

```json
{
  "title": "Too Many Requests",
  "message": "You have exceeded the rate limit. Please retry after the rate limit window resets."
}
```

**Rate Limit Headers:**

| Header | Type | Description |
|--------|------|-------------|
| `X-RateLimit-Limit` | integer | The maximum number of requests allowed in the current window |
| `X-RateLimit-Remaining` | integer | The number of requests remaining in the current window |
| `X-RateLimit-Reset` | integer | The number of seconds until the rate limit window resets |

### 500 Internal Server Error

Returned when an unexpected error occurs on the Calendly server.

```json
{
  "title": "Internal Server Error",
  "message": "An unexpected error occurred. Please try again later."
}
```

---

## Enums

### `kind` (Event Type)

| Value | Description |
|-------|-------------|
| `solo` | A one-on-one event type with a single host |
| `group` | A group event type that allows multiple invitees per time slot |

[UNDOCUMENTED - verify full enum against live API; additional values may exist]

### `pooling_type`

| Value | Description |
|-------|-------------|
| `round_robin` | Invitees are distributed among team members using round-robin routing |
| `collective` | All team members must be available for the time slot to be offered |
| `null` | Not a team/pooled event type (solo event) |

### `booking_method`

| Value | Description |
|-------|-------------|
| `poll` | Invitees vote on preferred times before a final time is selected |
| `null` | Standard instant booking |

[UNDOCUMENTED - verify full enum against live API]

### `locations[].kind` (this endpoint's subset)

| Value | Description |
|-------|-------------|
| `ask_invitee` | Invitee chooses or provides the location |
| `physical` | A physical address/location |
| `phone_call` | Phone call (host or invitee calls) |
| `zoom` | Zoom video conferencing |
| `google_hangouts` | Google Meet / Hangouts |
| `microsoft_teams` | Microsoft Teams |
| `gotomeeting` | GoToMeeting |
| `webex` | Cisco Webex |
| `custom` | Custom location with free-text |

> For the full harmonized superset of all known location kinds (including aliases and alias resolution rules), see the [Location Kind Enum in events.md](events.md#location-kind-enum-canonical-superset).

[UNDOCUMENTED - verify full enum against live API; additional conferencing integrations may exist]

### `custom_questions[].type`

| Value | Description |
|-------|-------------|
| `text` | Free-text single or multi-line input |
| `dropdown` | Dropdown selection from `answer_choices` |
| `radio` | Radio button selection from `answer_choices` |
| `checkbox` | Checkbox multi-selection from `answer_choices` |
| `phone_number` | Phone number input field |

[UNDOCUMENTED - verify full enum against live API]

### `profile.type`

| Value | Description |
|-------|-------------|
| `User` | Owned by an individual user |
| `Team` | Owned by a team/organization |

[UNDOCUMENTED - verify full enum against live API]

### `available_times[].status`

| Value | Description |
|-------|-------------|
| `available` | The time slot is open for booking |

[UNDOCUMENTED - verify if other status values exist (e.g., `"booked"`, `"tentative"`)]

---

## Pagination

The `GET /event_types` endpoint uses **cursor-based pagination**.

### How It Works

1. Make an initial request with the desired `count` (default: 20, max: 100).
2. The response includes a `pagination` object with `next_page_token` and `previous_page_token`.
3. Pass the `next_page_token` value as the `page_token` query parameter in the next request to retrieve the following page.
4. When `next_page_token` is `null`, you have reached the last page.

### Pagination Object Schema

| Field | Type | Description |
|-------|------|-------------|
| `count` | integer | Number of results in the current page |
| `next_page` | string \| null | Full URL for the next page (convenience field) |
| `previous_page` | string \| null | Full URL for the previous page (convenience field) |
| `next_page_token` | string \| null | Token to pass as `page_token` for the next page. `null` on the last page. |
| `previous_page_token` | string \| null | Token to pass as `page_token` for the previous page. `null` on the first page. |

### Example: Iterating Through All Pages

```bash
# Page 1
curl -s 'https://api.calendly.com/event_types?user=https://api.calendly.com/users/AAAA&count=20' \
  -H 'Authorization: Bearer TOKEN'
# Response includes: "next_page_token": "sNjq4TvMDfUHEl7zHRR0k0E1PCEJWvdi"

# Page 2
curl -s 'https://api.calendly.com/event_types?user=https://api.calendly.com/users/AAAA&count=20&page_token=sNjq4TvMDfUHEl7zHRR0k0E1PCEJWvdi' \
  -H 'Authorization: Bearer TOKEN'
# Response includes: "next_page_token": null  (last page)
```

### Endpoints Without Pagination

- `GET /event_types/{uuid}` -- Returns a single resource; no pagination.
- `GET /event_type_available_times` -- Returns all available times in the requested range; no pagination.

---

## CLI Reference

### `calendly event-types list`

List event types for the authenticated user.

```
calendly event-types list [OPTIONS]
```

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `--active` | flag | (omitted) | Show only active event types |
| `--inactive` | flag | (omitted) | Show only inactive event types |
| `--sort <FIELD:DIR>` | string | (API default) | Sort results. Example: `--sort name:asc` |
| `--count <N>` | integer | 20 | Number of results per page (1-100) |
| `--all` | flag | false | Auto-paginate and fetch all results |
| `--page-token <TOKEN>` | string | (none) | Retrieve a specific page by token |
| `--json` | flag | false | Output raw JSON response |
| `--user <URI>` | string | (current user) | User URI to list event types for. Defaults to the authenticated user's URI. |

**Examples:**

```bash
# List all active event types
calendly event-types list --active

# List all event types, sorted by name
calendly event-types list --sort name:asc --all

# List with custom page size
calendly event-types list --count 50

# Output as JSON
calendly event-types list --json
```

### `calendly event-types show`

Get the full details of a specific event type.

```
calendly event-types show <uuid>
```

| Argument | Type | Required | Description |
|----------|------|----------|-------------|
| `uuid` | string | Yes | The UUID of the event type (extracted from the event type URI) |

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `--json` | flag | false | Output raw JSON response |

**Examples:**

```bash
# Show event type details
calendly event-types show 88323028-5ef5-448b-9776-bc0c71b062a4

# Show as JSON
calendly event-types show 88323028-5ef5-448b-9776-bc0c71b062a4 --json
```

### `calendly event-types available-times`

Get available time slots for an event type within a date range.

```
calendly event-types available-times <uuid> --start <ISO8601> --end <ISO8601>
```

| Argument | Type | Required | Description |
|----------|------|----------|-------------|
| `uuid` | string | Yes | The UUID of the event type |

| Option | Type | Required | Description |
|--------|------|----------|-------------|
| `--start <ISO8601>` | string | No | Start of the time range (ISO 8601, must be in the future). **CLI default:** current time rounded up to next 15-minute boundary. |
| `--end <ISO8601>` | string | No | End of the time range (ISO 8601, max 7 days after start). **CLI default:** `start + 7 days` (the API maximum). |
| `--date <YYYY-MM-DD>` | string | No | Shorthand: sets start to beginning of the given day (in user's timezone) and end to end of that day. |
| `--today` | flag | No | Shorthand: sets start to beginning of today, end to end of today. |
| `--tomorrow` | flag | No | Shorthand: sets start to beginning of tomorrow, end to end of tomorrow. |
| `--this-week` | flag | No | Shorthand: sets start to now, end to end of current week (Sunday). |
| `--json` | flag | No | Output raw JSON response |

> **CLI default behavior:** When `--start` and `--end` are omitted, the CLI defaults to `start=now` (rounded to next 15-minute boundary) and `end=now+7d`. The API requires both `start_time` and `end_time`, so the CLI always populates them.

**Examples:**

```bash
# Get available times for next week
calendly event-types available-times 88323028-5ef5-448b-9776-bc0c71b062a4 \
  --start 2024-02-16T00:00:00Z \
  --end 2024-02-20T00:00:00Z

# Output as JSON
calendly event-types available-times 88323028-5ef5-448b-9776-bc0c71b062a4 \
  --start 2024-02-16T00:00:00Z \
  --end 2024-02-20T00:00:00Z \
  --json
```
