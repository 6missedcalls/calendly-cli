# Activity Log Module API Specification

Base URL: `https://api.calendly.com`

---

## 1. List Activity Log Entries

### Endpoint

```
GET /activity_log_entries
```

### Description

Retrieves a collection of activity log entries based on specified criteria. Supports filtering by organization, action, actor, namespace, search term, and time range, as well as cursor-based pagination to manage large result sets.

**Important:** The `organization` query parameter is auto-populated by the CLI from the cached current user's `current_organization` field when `--org` is not explicitly provided. `[DEFERRED TO QA]` -- Raw Calendly docs list this as Optional; verify against live API.

### Authentication

- **Header:** `Authorization: Bearer <personal_access_token>`

### Parameters

#### Query Parameters

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `organization` | string (URI) | Optional (auto-populated by CLI) | The URI of the organization to filter activity logs by. The CLI auto-populates this from the cached user profile (`current_organization`). `[DEFERRED TO QA]` -- Raw Calendly docs mark this as Optional. Verify whether omitting it returns unscoped results or an error. |
| `action` | string | Optional | Filters activity logs by a specific action performed (e.g., `"Add"`, `"Remove"`) |
| `actor` | string (URI) | Optional | Filters activity logs by the URI of the actor who performed the action |
| `count` | integer | Optional | The number of rows to return. Range: 1-100. Defaults to `20`. Although the API may accept count=0, the CLI enforces a minimum of 1. |
| `max_occurred_at` | string (ISO 8601 datetime) | Optional | Filters activity logs that occurred at or before this date and time |
| `min_occurred_at` | string (ISO 8601 datetime) | Optional | Filters activity logs that occurred at or after this date and time |
| `namespace` | string | Optional | Filters activity logs by a specific namespace (e.g., `"User"`, `"EventType"`, `"Organization"`) |
| `page_token` | string | Optional | A token to retrieve a specific page of results for pagination |
| `search_term` | string | Optional | A general search term to find relevant activity log entries |
| `sort` | string | Optional | Specifies the sorting order of the results. See `sort` enum below |

**`sort` Enum Values:**

| Value | Description |
|-------|-------------|
| `"occurred_at:asc"` | Sort by occurrence time, ascending (oldest first) |
| `"occurred_at:desc"` | Sort by occurrence time, descending (newest first) |

### Request Examples

#### Basic request (all entries for the organization)

```bash
curl --request GET \
  --url 'https://api.calendly.com/activity_log_entries?organization=https://api.calendly.com/organizations/AAAAAAAAAAAAAAA' \
  --header 'Authorization: Bearer YOUR_TOKEN' \
  --header 'Content-Type: application/json'
```

#### Filter by action and namespace

```bash
curl --request GET \
  --url 'https://api.calendly.com/activity_log_entries?organization=https://api.calendly.com/organizations/AAAAAAAAAAAAAAA&action=Add&namespace=User' \
  --header 'Authorization: Bearer YOUR_TOKEN' \
  --header 'Content-Type: application/json'
```

#### Filter by time range with sorting

```bash
curl --request GET \
  --url 'https://api.calendly.com/activity_log_entries?organization=https://api.calendly.com/organizations/AAAAAAAAAAAAAAA&min_occurred_at=2025-01-01T00:00:00.000Z&max_occurred_at=2025-12-31T23:59:59.999Z&sort=occurred_at:desc&count=50' \
  --header 'Authorization: Bearer YOUR_TOKEN' \
  --header 'Content-Type: application/json'
```

#### Filter by actor

```bash
curl --request GET \
  --url 'https://api.calendly.com/activity_log_entries?organization=https://api.calendly.com/organizations/AAAAAAAAAAAAAAA&actor=https://api.calendly.com/users/SDLKJENFJKD123' \
  --header 'Authorization: Bearer YOUR_TOKEN' \
  --header 'Content-Type: application/json'
```

#### Search by term

```bash
curl --request GET \
  --url 'https://api.calendly.com/activity_log_entries?organization=https://api.calendly.com/organizations/AAAAAAAAAAAAAAA&search_term=testuser' \
  --header 'Authorization: Bearer YOUR_TOKEN' \
  --header 'Content-Type: application/json'
```

### Response

#### Success Response (200)

> **Note:** This response extends the standard paginated collection format with three additional top-level fields (`last_event_time`, `total_count`, `exceeds_max_total_count`). The implementation must NOT use a generic `PaginatedResponse<T>` for this endpoint -- use a dedicated response struct.

| Field | Type | Description |
|-------|------|-------------|
| `collection` | array | Array of Activity Log Entry objects |
| `pagination` | object | Pagination metadata for cursor-based navigation |
| `last_event_time` | string (ISO 8601 datetime) or null | The date and time of the newest entry in the collection. `null` if the collection is empty |
| `total_count` | integer | Total number of records found based on search criteria |
| `exceeds_max_total_count` | boolean | Indicates if the total number of results exceeds the `total_count` field value |

**Activity Log Entry Object:**

| Field | Type | Description |
|-------|------|-------------|
| `occurred_at` | string (ISO 8601 datetime) | The date and time when the activity occurred |
| `uri` | string (URI) | The unique URI of the activity log entry |
| `namespace` | string | The namespace of the action (e.g., `"User"`, `"EventType"`) |
| `action` | string | The action performed (e.g., `"Add"`, `"Remove"`) |
| `actor` | object (Activity Log Actor) | Object describing the user or system that performed the action |
| `fully_qualified_name` | string | The fully qualified name of the action, formatted as `"Namespace.Action"` (e.g., `"User.Add"`) |
| `details` | object | Additional details about the activity. Structure varies by action type |
| `organization` | string (URI) | The URI of the organization associated with the activity |

**Activity Log Actor Object:**

| Field | Type | Description |
|-------|------|-------------|
| `uri` | string (URI) | The URI of the actor |
| `type` | string | The type of the actor (e.g., `"User"`) |
| `organization` | object \| null | Details about the actor's organization. May be absent or `null` for system-initiated actions. See Actor Organization below. |
| `group` | object \| null | Details about the actor's group. May be absent or `null` for actors not assigned to a group. See Actor Group below. |
| `display_name` | string | The display name of the actor |
| `alternative_identifier` | string | An alternative identifier for the actor (e.g., email address) |

**Actor Organization Object:**

| Field | Type | Description |
|-------|------|-------------|
| `uri` | string (URI) | The URI of the actor's organization |
| `role` | string | The actor's role within the organization (e.g., `"Owner"`, `"Admin"`, `"User"`) |

**Actor Group Object:**

| Field | Type | Description |
|-------|------|-------------|
| `uri` | string (URI) | The URI of the group |
| `name` | string | The name of the group (e.g., `"Development"`) |
| `role` | string | The actor's role within the group (e.g., `"Admin"`, `"Member"`) |

**Pagination Object:**

| Field | Type | Description |
|-------|------|-------------|
| `count` | integer | The number of items on the current page |
| `next_page` | string (URI) or null | Full URI for the next page of results. `null` if no next page |
| `previous_page` | string (URI) or null | Full URI for the previous page of results. `null` if no previous page |
| `next_page_token` | string or null | Token for retrieving the next page of results. `null` if no next page |
| `previous_page_token` | string or null | Token for retrieving the previous page of results. `null` if no previous page |

#### Response Example

```json
{
  "collection": [
    {
      "occurred_at": "2020-01-02T03:04:05.678Z",
      "uri": "https://api.calendly.com/activity_log_entries/ALFKJELNCLKSJDLKFJGELKJ",
      "namespace": "User",
      "action": "Add",
      "actor": {
        "uri": "https://api.calendly.com/users/SDLKJENFJKD123",
        "type": "User",
        "organization": {
          "uri": "https://api.calendly.com/organizations/LKJENFLKE293847",
          "role": "Owner"
        },
        "group": {
          "uri": "https://api.calendly.com/groups/123987DJLKJEF",
          "name": "Development",
          "role": "Admin"
        },
        "display_name": "Test User",
        "alternative_identifier": "testuser@example.com"
      },
      "fully_qualified_name": "User.Add",
      "details": {},
      "organization": "https://api.calendly.com/organizations/AAAAAAAAAAAAAAA"
    }
  ],
  "pagination": {
    "count": 20,
    "next_page": "https://api.calendly.com/activity_log?page_token=sNjq4TvMDfUHEl7zHRR0k0E1PCEJWvdi",
    "next_page_token": "sNjq4TvMDfUHEl7zHRR0k0E1PCEJWvdi",
    "previous_page": "https://api.calendly.com/activity_log?page_token=dGhpcyBpcyBvbmx5IGFuIGV4YW1wbGUgc3RyaW5n",
    "previous_page_token": "dGhpcyBpcyBvbmx5IGFuIGV4YW1wbGUgc3RyaW5n"
  },
  "last_event_time": "2020-01-02T03:04:05.678Z",
  "total_count": 1,
  "exceeds_max_total_count": false
}
```

### Error Responses

#### 401 Unauthorized

Returned when the Bearer token is missing or invalid.

```json
{
  "title": "Unauthenticated",
  "message": "The access token is expired"
}
```

#### 403 Forbidden

Returned when the authenticated user does not have permission to access activity logs for the specified organization.

```json
{
  "title": "Permission Denied",
  "message": "You do not have permission to access this resource"
}
```

#### 400 Bad Request

Returned when query parameters are invalid (e.g., `count` exceeds 100, invalid datetime format).

```json
{
  "title": "Invalid Argument",
  "message": "The supplied parameters are invalid"
}
```

#### 422 Unprocessable Entity

Returned when the request is syntactically valid but semantically incorrect (e.g., invalid URI format for `organization`).

```json
{
  "title": "Unprocessable Entity",
  "message": "The organization URI is not valid."
}
```

#### 429 Too Many Requests

Returned when the API rate limit has been exceeded.

```json
{
  "title": "Too Many Requests",
  "message": "You have exceeded the rate limit"
}
```

#### 500 Internal Server Error

Returned when an unexpected server-side error occurs.

```json
{
  "title": "Internal Server Error",
  "message": "An unexpected error occurred. Please try again later."
}
```

### Rate Limits

Calendly enforces rate limits on API requests. The standard rate limit is applied per access token. If you exceed the limit, you will receive a `429` response. Implement exponential backoff when retrying.

**Rate Limit Response Headers:**

| Header | Type | Description |
|--------|------|-------------|
| `X-RateLimit-Limit` | integer | Maximum number of requests allowed in the current window |
| `X-RateLimit-Remaining` | integer | Number of requests remaining in the current window |
| `X-RateLimit-Reset` | integer | Number of seconds until the rate limit window resets |

### Pagination

- **Format:** Cursor-based pagination via `page_token` query parameter.
- **Max `count`:** 100
- **Default `count`:** 20
- To fetch the next page, use the `next_page_token` value from the response as the `page_token` query parameter in the subsequent request.
- When `next_page_token` is `null`, there are no more pages to fetch.
- The `total_count` field indicates the total number of matching records. If `exceeds_max_total_count` is `true`, the actual total exceeds the reported `total_count`.

### CLI Usage

```
calendly activity-log list [OPTIONS]
```

#### Options

| Flag | Type | Description |
|------|------|-------------|
| `--org <uri>` | string | Organization URI. Auto-populated from cached user's `current_organization` if omitted |
| `--action <action>` | string | Filter by action (e.g., `"Add"`, `"Remove"`) |
| `--actor <uri>` | string | Filter by actor URI |
| `--namespace <ns>` | string | Filter by namespace (e.g., `"User"`, `"EventType"`) |
| `--search <term>` | string | Search term to filter entries |
| `--since <datetime>` | string (ISO 8601) | Filter entries that occurred at or after this time (`min_occurred_at`) |
| `--until <datetime>` | string (ISO 8601) | Filter entries that occurred at or before this time (`max_occurred_at`) |
| `--sort <sort>` | string | Sort order: `"occurred_at:asc"` or `"occurred_at:desc"` |
| `--count <N>` | integer | Number of results per page (1-100, default 20) |
| `--all` | flag | Automatically paginate through all results instead of returning a single page |

#### CLI Examples

```bash
# List recent activity logs (uses cached organization)
calendly activity-log list

# List all user-related activity
calendly activity-log list --namespace User

# Search for a specific user's actions
calendly activity-log list --search "testuser@example.com"

# List activity within a date range, sorted newest first
calendly activity-log list --since 2025-01-01T00:00:00Z --until 2025-12-31T23:59:59Z --sort occurred_at:desc

# Filter by actor and action
calendly activity-log list --actor https://api.calendly.com/users/SDLKJENFJKD123 --action Add

# Fetch all results with automatic pagination
calendly activity-log list --all --count 100

# Specify a different organization
calendly activity-log list --org https://api.calendly.com/organizations/BBBBBBBBBBBBBBB
```

#### CLI Parameter Mapping

| CLI Flag | API Query Parameter |
|----------|-------------------|
| `--org` | `organization` |
| `--action` | `action` |
| `--actor` | `actor` |
| `--namespace` | `namespace` |
| `--search` | `search_term` |
| `--since` | `min_occurred_at` |
| `--until` | `max_occurred_at` |
| `--sort` | `sort` |
| `--count` | `count` |
| `--all` | (internal: auto-paginate using `page_token`) |

### Notes

1. The `organization` parameter is auto-populated by the CLI from the cached user profile (`current_organization` from `GET /users/me`). `[DEFERRED TO QA]` -- Raw Calendly docs mark this as Optional. Verify whether omitting it returns unscoped results or a 400 error. Users can override with the `--org` flag.
2. The `details` object has a dynamic structure that varies depending on the `namespace` and `action` of the entry. Consumers should treat it as an opaque JSON object.
3. The `fully_qualified_name` field is a convenience field combining `namespace` and `action` in the format `"Namespace.Action"` (e.g., `"User.Add"`).
4. The `exceeds_max_total_count` boolean indicates that the API has capped the `total_count` value. When `true`, the actual number of matching records is greater than the reported `total_count`.
5. The `last_event_time` field reflects the timestamp of the newest entry in the current result set, not across all pages. It is `null` when the collection is empty.
6. When using `--all`, the CLI iterates through pages using `next_page_token` until it is `null`, collecting all results before displaying them.
7. Actor `type` is typically `"User"` but may differ for system-initiated actions.
8. The `alternative_identifier` on the actor is generally the actor's email address.
