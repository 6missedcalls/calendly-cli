# Organizations Module API Specification

Base URL: `https://api.calendly.com`

---

## 1. List Organization Invitations

### Endpoint

```
GET /organizations/{uuid}/invitations
```

### Description

Retrieves a list of organization invitations for a specified organization. This endpoint supports filtering by invitation `status` and invited `email`, as well as sorting and cursor-based pagination using `count` and `page_token`. The organization UUID is obtained from the current user's `current_organization` field, which is returned by the `GET /users/me` endpoint. The CLI resolves this automatically so the user does not need to supply the UUID manually.

### Authentication

- **Header:** `Authorization: Bearer <personal_access_token>`

### Parameters

#### Path Parameters

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| `uuid` | string | Yes | The unique identifier of the organization. Extracted from the `current_organization` URI returned by `GET /users/me`. |

#### Query Parameters

| Parameter | Type | Required | Default | Description |
|-----------|------|----------|---------|-------------|
| `count` | integer | No | 20 | The number of rows to return. Must be between 0 and 100. |
| `email` | string | No | — | Filters invitations by the email address of the invited person. |
| `page_token` | string | No | — | Token to retrieve the next page of results. Obtained from a previous response's `pagination.next_page_token` or `pagination.previous_page_token`. |
| `sort` | string | No | — | Specifies the order of the results (e.g., `created_at`). |
| `status` | string | No | — | Filters invitations by their status. See enum below. |

**`status` Enum Values:**

| Value | Description |
|-------|-------------|
| `"pending"` | Invitation has been sent but not yet responded to |
| `"accepted"` | Invitation has been accepted by the invitee |
| `"declined"` | Invitation has been declined by the invitee |

### Request Example

```shell
curl --request GET \
  --url 'https://api.calendly.com/organizations/AAAAAAAAAAAAAAAA/invitations?status=pending&count=20' \
  --header 'Authorization: Bearer YOUR_ACCESS_TOKEN' \
  --header 'Content-Type: application/json'
```

### Response

#### Success Response (200)

The response is a JSON object containing a `collection` array and a `pagination` object.

**Top-Level Response:**

| Field | Type | Description |
|-------|------|-------------|
| `collection` | array[Organization Invitation] | A list of organization invitation objects |
| `pagination` | Pagination | Contains information about the pagination of the results |

**Organization Invitation Object:**

| Field | Type | Description |
|-------|------|-------------|
| `uri` | string | Canonical reference (unique identifier) for the organization invitation |
| `organization` | string | Canonical reference (unique identifier) for the organization |
| `email` | string | The email address of the invited person |
| `status` | string | The status of the invitation (`pending`, `accepted`, or `declined`) |
| `created_at` | string (ISO 8601) | The moment the invitation was created |
| `updated_at` | string (ISO 8601) | The moment the invitation was last updated |
| `last_sent_at` | string or null | The moment the invitation was last sent. Null if no send has been recorded. |
| `user` | string or null | When the invitation is accepted, a canonical reference (URI) to the user who accepted it. Null while the invitation is pending or declined. |

**Pagination Object:**

| Field | Type | Description |
|-------|------|-------------|
| `count` | integer | The number of rows returned in this response |
| `next_page` | string or null | Full URI to return the next page of results. Null if there is no next page. |
| `previous_page` | string or null | Full URI to return the previous page of results. Null if there is no previous page. |
| `next_page_token` | string or null | Token to return the next page of results. Pass this as the `page_token` query parameter. Null if there is no next page. |
| `previous_page_token` | string or null | Token to return the previous page of results. Pass this as the `page_token` query parameter. Null if there is no previous page. |

#### Response Example

```json
{
  "collection": [
    {
      "uri": "https://api.calendly.com/organizations/AAAAAAAAAAAAAAAA/invitations/BBBBBBBBBBBBBBBB",
      "organization": "https://api.calendly.com/organizations/AAAAAAAAAAAAAAAA",
      "email": "test@example.com",
      "status": "accepted",
      "created_at": "2019-08-07T06:05:04.321123Z",
      "updated_at": "2019-01-02T03:04:05.678123Z",
      "last_sent_at": "2019-01-02T03:04:05.678123Z",
      "user": "https://api.calendly.com/users/AAAAAAAAAAAAAAAA"
    }
  ],
  "pagination": {
    "count": 20,
    "next_page": "https://api.calendly.com/organizations/AAAAAAAAAAAAAAAA/invitations?count=1&page_token=sNjq4TvMDfUHEl7zHRR0k0E1PCEJWvdi",
    "previous_page": "https://api.calendly.com/organizations/AAAAAAAAAAAAAAAA/invitations?count=1&page_token=VJs2rfDYeY8ahZpq0QI1O114LJkNjd7H",
    "next_page_token": "sNjq4TvMDfUHEl7zHRR0k0E1PCEJWvdi",
    "previous_page_token": "VJs2rfDYeY8ahZpq0QI1O114LJkNjd7H"
  }
}
```

### Error Responses

#### 401 Unauthorized

Returned when the bearer token is missing or invalid.

```json
{
  "title": "Unauthenticated",
  "message": "The access token is expired"
}
```

#### 403 Forbidden

Returned when the authenticated user does not have permission to view invitations for the specified organization.

```json
{
  "title": "Permission Denied",
  "message": "You do not have permission to access this resource"
}
```

#### 404 Not Found

Returned when the organization UUID does not exist.

```json
{
  "title": "Resource Not Found",
  "message": "The organization could not be found"
}
```

#### 429 Too Many Requests

Returned when the rate limit has been exceeded.

### Rate Limits

Rate limits are determined by your Calendly plan. Requests that exceed the limit receive a `429` response. Implement exponential backoff when retrying.

### Pagination

Cursor-based. Use the `page_token` query parameter with a token from `pagination.next_page_token` or `pagination.previous_page_token` to navigate between pages. The `count` parameter controls page size (max 100, default 20). To fetch all results, continue requesting the next page until `pagination.next_page_token` is null.

### CLI Usage

```
calendly org invitations [--status <s>] [--email <e>] [--count N] [--all]
```

| Flag | Type | Description |
|------|------|-------------|
| `--status <s>` | string | Filter by invitation status (`pending`, `accepted`, `declined`) |
| `--email <e>` | string | Filter by the invited person's email address |
| `--count N` | integer | Number of results per page (1-100, default 20) |
| `--all` | flag | Automatically paginate through all results and return the complete list |

The CLI automatically resolves the organization UUID from the authenticated user's profile (`GET /users/me` -> `current_organization`). The `--all` flag handles cursor-based pagination internally by following `next_page_token` until all pages have been fetched.

### Notes

1. The organization UUID is not something the user supplies directly. The CLI extracts it from the `current_organization` URI returned by `GET /users/me`. For example, if `current_organization` is `https://api.calendly.com/organizations/AAAAAAAAAAAAAAAA`, the UUID is `AAAAAAAAAAAAAAAA`.
2. The `uri`, `organization`, and `user` fields are full Calendly API URIs, not bare UUIDs. Implementations should parse the UUID from the trailing path segment when needed.
3. The `user` field is only populated when `status` is `accepted`. For `pending` and `declined` invitations, this field is null.
4. The `last_sent_at` field may be null if the invitation has not yet been delivered.
5. When using `--all`, the CLI issues multiple requests sequentially, following `next_page_token` until it is null, then merges all `collection` arrays into a single output.
6. The `sort` parameter accepts field names such as `created_at`. Prefix with `-` for descending order (e.g., `-created_at`) if supported by the API.
