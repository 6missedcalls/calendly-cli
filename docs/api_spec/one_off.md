# One-Off Event Types Module API Specification

Base URL: `https://api.calendly.com`

---

## Table of Contents

- [Authentication](#authentication)
- [Rate Limits](#rate-limits)
- [Common Error Responses](#common-error-responses)
- [1. Create One-Off Event Type](#1-create-one-off-event-type)

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

## 1. Create One-Off Event Type

### Description

Creates a one-off event type, which is a single-use scheduling link for a specific meeting. One-off event types are designed for ad hoc meetings that do not repeat -- unlike standard event types, they are typically used once and tied to a specific date range. The response includes a `scheduling_url` that can be shared with invitees to book the meeting.

### Method

```
POST /one_off_event_types
```

### Endpoint

```
https://api.calendly.com/one_off_event_types
```

### Path Parameters

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| -- | -- | -- | This endpoint has no path parameters |

### Query Parameters

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| -- | -- | -- | This endpoint has no query parameters |

### Request Body

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `name` | string | Yes | The name of the one-off event type (e.g., `"My Meeting"`) |
| `host` | string | Yes | The URI of the user who will host the event (e.g., `https://api.calendly.com/users/AAAAAAAAAAAAAAAA`). Obtain from `GET /users/me` |
| `co_hosts` | array of strings | No | An array of user URIs for co-hosts of the event. Each element is a full API URI (e.g., `https://api.calendly.com/users/BBBBBBBBBBBBBBBB`) |
| `duration` | integer | Yes | The duration of the meeting in minutes (e.g., `30`) |
| `timezone` | string | Yes | The IANA timezone identifier for the event (e.g., `"America/New_York"`, `"Europe/London"`) |
| `date_setting` | object | Yes | Configuration for the available date range. See [date_setting Object](#date_setting-object) |
| `location` | object | Yes | Configuration for the meeting location. See [location Object](#location-object) |

#### `date_setting` Object

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `type` | string | Yes | The type of date constraint. Currently only `"date_range"` is supported |
| `start_date` | string | Yes | The start date of the available booking window in `YYYY-MM-DD` format (e.g., `"2020-01-07"`) |
| `end_date` | string | Yes | The end date of the available booking window in `YYYY-MM-DD` format (e.g., `"2020-01-09"`). Must be on or after `start_date` |

#### `location` Object

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `kind` | string | Yes | The type of location for the meeting. See [location kind Enum Values](#location-kind-enum-values) |
| `location` | string | No | Specific location details. Required for `"physical"` kind (e.g., `"Main Office"`). Ignored for virtual conference types where the platform auto-generates the link |
| `additional_info` | string | No | Additional information about the location (e.g., parking instructions, building access details). **Note:** The raw Calendly API documentation contains a typo, spelling this field as `"additonal_info"` (missing the second 'i'). The actual API field name must be verified against a live request -- implementations should support both spellings defensively |

#### `location` `kind` Enum Values

| Value | Description |
|-------|-------------|
| `"physical"` | An in-person meeting at a physical address |
| `"zoom_conference"` | A Zoom video conference (link auto-generated by Calendly) |
| `"google_conference"` | A Google Meet video conference (link auto-generated by Calendly) |
| `"microsoft_teams_conference"` | A Microsoft Teams video conference (link auto-generated by Calendly) |
| `"outbound_call"` | The host calls the invitee |
| `"inbound_call"` | The invitee calls the host |
| `"custom"` | A custom location specified in the `location` field |

> **Note:** The full set of supported `kind` values is not exhaustively documented in the raw API docs. The values above are inferred from the Calendly API ecosystem. Verify the complete set against the live API if additional location types are needed.

### Request Example

**cURL:**

```bash
curl --request POST \
  --url https://api.calendly.com/one_off_event_types \
  --header 'Authorization: Bearer {token}' \
  --header 'Content-Type: application/json' \
  --data '{
  "name": "My Meeting",
  "host": "https://api.calendly.com/users/AAAAAAAAAAAAAAAA",
  "co_hosts": [
    "https://api.calendly.com/users/BBBBBBBBBBBBBBBB",
    "https://api.calendly.com/users/CCCCCCCCCCCCCCCC"
  ],
  "duration": 30,
  "timezone": "America/New_York",
  "date_setting": {
    "type": "date_range",
    "start_date": "2020-01-07",
    "end_date": "2020-01-09"
  },
  "location": {
    "kind": "physical",
    "location": "Main Office",
    "additonal_info": "Enter through the south lobby"
  }
}'
```

**JSON Request Body:**

```json
{
  "name": "My Meeting",
  "host": "https://api.calendly.com/users/AAAAAAAAAAAAAAAA",
  "co_hosts": [
    "https://api.calendly.com/users/BBBBBBBBBBBBBBBB",
    "https://api.calendly.com/users/CCCCCCCCCCCCCCCC"
  ],
  "duration": 30,
  "timezone": "America/New_York",
  "date_setting": {
    "type": "date_range",
    "start_date": "2020-01-07",
    "end_date": "2020-01-09"
  },
  "location": {
    "kind": "physical",
    "location": "Main Office",
    "additonal_info": "Enter through the south lobby"
  }
}
```

### Response Body

#### Success Response (201 Created)

> **UNDOCUMENTED:** The exact response schema for this endpoint is not fully documented in the raw Calendly API docs. The following structure is inferred from the standard Calendly event type resource pattern (see `GET /event_types`). Verify all fields against the live API before relying on them in production.

| Field | Type | Nullable | Description |
|-------|------|----------|-------------|
| `resource` | object | No | Wrapper object containing the created one-off event type |
| `resource.uri` | string | No | Canonical API URI for the created event type (e.g., `https://api.calendly.com/event_types/{uuid}`) |
| `resource.name` | string | No | The name of the event type |
| `resource.scheduling_url` | string | No | The public URL that can be shared with invitees to book this one-off event |
| `resource.duration` | integer | No | The duration of the meeting in minutes |
| `resource.kind` | string | No | The kind of event type (expected: `"solo"` or similar) |
| `resource.slug` | string | No | The unique slug for the event type |
| `resource.secret` | boolean | No | Whether the event type is secret (not publicly listed) |
| `resource.created_at` | string | No | ISO 8601 timestamp of when the event type was created |
| `resource.updated_at` | string | No | ISO 8601 timestamp of the last update to the event type |

### Response Example

> **UNDOCUMENTED:** This response example is a best-effort reconstruction based on the standard Calendly event type resource format. Verify against the live API.

```json
{
  "resource": {
    "uri": "https://api.calendly.com/event_types/a1b2c3d4-e5f6-7890-abcd-ef1234567890",
    "name": "My Meeting",
    "scheduling_url": "https://calendly.com/d/abc-def-ghi/my-meeting",
    "duration": 30,
    "kind": "solo",
    "slug": "my-meeting",
    "secret": false,
    "created_at": "2020-01-06T12:00:00.000000Z",
    "updated_at": "2020-01-06T12:00:00.000000Z"
  }
}
```

### Error Codes

In addition to the [Common Error Responses](#common-error-responses), this endpoint may return:

| Status | Error | Description |
|--------|-------|-------------|
| 400 | Invalid Argument | A required field is missing, a field value is invalid (e.g., `end_date` before `start_date`, non-positive `duration`), or the request body is not valid JSON |
| 401 | Unauthenticated | The Bearer token is missing, expired, or revoked |
| 403 | Permission Denied | The authenticated user does not have permission to create event types for the specified `host`, or the user's plan does not support one-off event types |
| 409 | Conflict | A one-off event type with the same parameters already exists (if applicable -- verify against live API) |
| 422 | Unprocessable Entity | The request body is syntactically valid but semantically invalid (e.g., a co-host URI that does not exist, an unsupported `location.kind`) |

### Pagination

This endpoint creates a single resource and does not support pagination.

### CLI Command Mapping

This endpoint is used by the following CLI command:

```
calendly one-off create --name <name> --duration <min> --timezone <tz> --start-date <YYYY-MM-DD> --end-date <YYYY-MM-DD> --location-kind <kind> [--location <details>] [--additional-info <info>] [--co-host <uri>...]
```

**CLI Options:**

| Flag | Required | Default | Description |
|------|----------|---------|-------------|
| `--name` | Yes | -- | The name of the one-off event type |
| `--duration` | Yes | -- | The duration of the meeting in minutes |
| `--timezone` | Yes | -- | IANA timezone identifier (e.g., `America/New_York`) |
| `--start-date` | Yes | -- | Start date of the booking window (`YYYY-MM-DD`) |
| `--end-date` | Yes | -- | End date of the booking window (`YYYY-MM-DD`) |
| `--location-kind` | Yes | -- | Location type (e.g., `physical`, `zoom_conference`) |
| `--location` | No | -- | Location details (required for `physical` and `custom` kinds) |
| `--additional-info` | No | -- | Extra information about the location |
| `--co-host` | No | -- | Co-host user URI. Can be specified multiple times for multiple co-hosts |
| `--host` | No | Current user URI | The host user URI. Defaults to the authenticated user obtained from `GET /users/me` (cached locally) |

**Behavior:**

1. If `--host` is not provided, the CLI resolves the host URI from cached user data (originally obtained via `GET /users/me`). If no cached data is available, the CLI calls `GET /users/me` first and caches the result.
2. Constructs a `POST /one_off_event_types` request with the provided parameters.
3. On success, displays the created event type's `scheduling_url` (the shareable booking link) and `uri`.
4. On error, displays the error title and message from the API response.

**Example Usage:**

```bash
# Create a 30-minute in-person meeting
calendly one-off create \
  --name "Project Kickoff" \
  --duration 30 \
  --timezone "America/New_York" \
  --start-date 2024-03-01 \
  --end-date 2024-03-15 \
  --location-kind physical \
  --location "123 Main St, Suite 400"

# Create a 60-minute Zoom meeting with co-hosts
calendly one-off create \
  --name "Quarterly Review" \
  --duration 60 \
  --timezone "Europe/London" \
  --start-date 2024-04-01 \
  --end-date 2024-04-05 \
  --location-kind zoom_conference \
  --co-host "https://api.calendly.com/users/BBBBBBBBBBBBBBBB" \
  --co-host "https://api.calendly.com/users/CCCCCCCCCCCCCCCC"
```

### Notes

1. One-off event types are single-use scheduling links intended for ad hoc meetings. They differ from standard event types, which are reusable and persistent.
2. The `host` field must be a valid Calendly user URI in the format `https://api.calendly.com/users/{uuid}`. The CLI auto-populates this from cached `GET /users/me` data when `--host` is omitted.
3. The `co_hosts` array is optional. When provided, each element must be a valid Calendly user URI. Co-hosts must belong to the same organization as the host.
4. The `date_setting.type` field currently only supports `"date_range"`. Future API versions may introduce additional types.
5. The `date_setting.start_date` and `date_setting.end_date` fields use `YYYY-MM-DD` format (not ISO 8601 datetime). The dates are interpreted in the timezone specified by the `timezone` field.
6. **Typo in raw API docs:** The Calendly API documentation spells the location field as `"additonal_info"` (missing the second 'i' in "additional"). Implementations should use the spelling `"additonal_info"` in the request body to match the API's expected field name, but the CLI flag uses the corrected spelling `--additional-info` for user-friendliness.
7. For virtual conference location kinds (`zoom_conference`, `google_conference`, `microsoft_teams_conference`), the conference link is auto-generated by Calendly -- the `location` field inside the location object can be omitted.
8. The response schema is **undocumented** in the raw Calendly API reference. The response fields documented above are inferred from the standard event type resource pattern used by `GET /event_types`. Always verify against the live API.
9. This endpoint does not support pagination, as it creates and returns a single resource.
