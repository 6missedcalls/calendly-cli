# Users Module API Specification

Base URL: `https://api.calendly.com`

---

## Table of Contents

- [Authentication](#authentication)
- [Rate Limits](#rate-limits)
- [Common Error Responses](#common-error-responses)
- [1. Get Current User](#1-get-current-user)

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

## 1. Get Current User

### Description

Retrieves the profile information of the currently authenticated user. This is the primary endpoint for obtaining the user's URI, organization URI, and account metadata. The user URI returned by this endpoint is required as a parameter for many other Calendly API endpoints (e.g., listing event types, listing scheduled events).

### Method

```
GET /users/me
```

### Endpoint

```
https://api.calendly.com/users/me
```

### Path Parameters

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| — | — | — | This endpoint has no path parameters |

### Query Parameters

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| — | — | — | This endpoint has no query parameters |

### Request Body

This endpoint does not accept a request body.

### Request Example

**cURL:**

```bash
curl --request GET \
  --url https://api.calendly.com/users/me \
  --header 'Authorization: Bearer {token}' \
  --header 'Content-Type: application/json'
```

### Response Body

#### Success Response (200)

| Field | Type | Nullable | Description |
|-------|------|----------|-------------|
| `resource` | object | No | Wrapper object containing the user resource |
| `resource.uri` | string | No | Canonical API URI for the user (e.g., `https://api.calendly.com/users/AAAAAAAAAAAAAAAA`) |
| `resource.name` | string | No | Full display name of the user |
| `resource.slug` | string | No | URL slug for the user's scheduling page |
| `resource.email` | string | No | Email address associated with the user's account |
| `resource.scheduling_url` | string | No | Public URL to the user's scheduling page (e.g., `https://calendly.com/acmesales`) |
| `resource.timezone` | string | No | IANA timezone identifier for the user (e.g., `America/New_York`) |
| `resource.time_notation` | string | No | Time format preference. One of `"12h"` or `"24h"` |
| `resource.avatar_url` | string | **Yes** | URL to the user's avatar image. `null` if no avatar is set |
| `resource.created_at` | string | No | ISO 8601 timestamp of when the user account was created |
| `resource.updated_at` | string | No | ISO 8601 timestamp of the last update to the user account |
| `resource.current_organization` | string | No | Canonical API URI for the user's current organization |
| `resource.resource_type` | string | No | Type of the resource. Always `"User"` for this endpoint |
| `resource.locale` | string | No | Language/locale setting for the user (e.g., `"en"`) |

### Response Example

```json
{
  "resource": {
    "uri": "https://api.calendly.com/users/AAAAAAAAAAAAAAAA",
    "name": "John Doe",
    "slug": "acmesales",
    "email": "test@example.com",
    "scheduling_url": "https://calendly.com/acmesales",
    "timezone": "America/New_York",
    "time_notation": "12h",
    "avatar_url": "https://01234567890.cloudfront.net/uploads/user/avatar/0123456/a1b2c3d4.png",
    "created_at": "2019-01-02T03:04:05.678123Z",
    "updated_at": "2019-08-07T06:05:04.321123Z",
    "current_organization": "https://api.calendly.com/organizations/AAAAAAAAAAAAAAAA",
    "resource_type": "User",
    "locale": "en"
  }
}
```

### Error Codes

In addition to the [Common Error Responses](#common-error-responses), this endpoint may return:

| Status | Error | Description |
|--------|-------|-------------|
| 401 | Unauthenticated | The Bearer token is missing, expired, or revoked |
| 403 | Permission Denied | The token does not have the required scope to access user information |

### Pagination

This endpoint returns a single resource and does not support pagination.

### CLI Command Mapping

This endpoint is used by the following CLI command:

```
calendly me
```

**Behavior:** Calls `GET /users/me` with the configured Bearer token and displays the authenticated user's profile information (name, email, timezone, scheduling URL, organization, etc.).

### Notes

1. The `uri` field returned in the response is the canonical user identifier used as a parameter in other Calendly API endpoints (e.g., `GET /event_types?user={uri}`).
2. The `current_organization` URI is required for organization-scoped API calls such as listing organization invitations or activity log entries.
3. The `avatar_url` field is the only nullable field in the response. It returns `null` when the user has not uploaded a profile image.
4. The `timezone` field uses IANA timezone database identifiers (e.g., `America/New_York`, `Europe/London`).
5. The `time_notation` field reflects the user's display preference and will be either `"12h"` (12-hour clock) or `"24h"` (24-hour clock).
