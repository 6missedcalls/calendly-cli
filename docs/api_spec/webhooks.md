# Webhooks Reference

Base URL: `https://api.calendly.com`

Source: https://developer.calendly.com/api-docs/d7755e2f9e5fe-calendly-api

---

## Table of Contents

- [1. Overview](#1-overview)
- [2. Webhook Signatures](#2-webhook-signatures)
- [3. Webhook Errors](#3-webhook-errors)
- [4. Webhook Timeouts](#4-webhook-timeouts)
- [5. CLI Decision](#5-cli-decision)

---

## 1. Overview

Calendly webhooks allow external applications to receive real-time notifications when specific events occur within a Calendly account. When a webhook subscription is active, Calendly sends an HTTP POST request to the subscriber's configured URL each time a matching event is triggered (e.g., an invitee schedules or cancels a meeting).

Webhook subscriptions are managed through the Calendly API and are scoped to either a user or an organization. The subscriber must provide a publicly-accessible HTTPS endpoint capable of receiving and responding to incoming POST requests from Calendly's servers.

**Key concepts:**

- **Subscription:** A registered callback URL paired with one or more event types to listen for.
- **Event payload:** The JSON body delivered to the callback URL when a subscribed event occurs.
- **Signing secret:** A shared secret used to verify that incoming webhook payloads originate from Calendly.

[UNDOCUMENTED - refer to https://developer.calendly.com/api-docs for full webhook subscription API, including supported event types, payload schemas, and subscription CRUD endpoints]

---

## 2. Webhook Signatures

Webhook signatures provide a mechanism for verifying the authenticity of incoming webhook events. When a webhook subscription is created with a signing secret, Calendly includes a cryptographic signature in the headers of each delivered payload. The receiving server should validate this signature before processing the event to ensure the request was not forged or tampered with.

**General verification flow:**

1. Extract the signature from the webhook request headers.
2. Compute an HMAC digest of the raw request body using the shared signing secret.
3. Compare the computed digest against the provided signature using a constant-time comparison to prevent timing attacks.
4. Reject the request if the signatures do not match.

[UNDOCUMENTED - refer to https://developer.calendly.com/api-docs for the specific signature header name, hashing algorithm (e.g., HMAC-SHA256), signature encoding format, and complete verification examples]

---

## 3. Webhook Errors

Calendly's webhook delivery system may encounter errors when attempting to deliver event payloads to a subscriber's endpoint. These errors can originate from the subscriber's server (e.g., returning a non-2xx HTTP status code) or from network-level issues (e.g., DNS resolution failure, connection refused).

**Expected error handling behavior:**

- Calendly may implement automatic retry logic for failed deliveries with exponential backoff.
- Subscriptions that consistently fail delivery may be automatically disabled after a threshold of consecutive failures.
- Subscribers should return a `2xx` HTTP status code promptly to acknowledge receipt of the webhook payload.

[UNDOCUMENTED - refer to https://developer.calendly.com/api-docs for the exact retry policy, maximum retry count, backoff intervals, automatic disabling thresholds, and error notification mechanisms]

---

## 4. Webhook Timeouts

Calendly enforces a timeout on webhook delivery requests. If the subscriber's endpoint does not respond within the allotted time, Calendly treats the delivery as failed and may retry according to its retry policy.

**General guidance:**

- Subscribers should respond to webhook requests as quickly as possible (ideally within a few seconds).
- Heavy processing should be deferred to a background job after acknowledging receipt with a `2xx` response.
- Responses that exceed the timeout threshold are treated as delivery failures.

[UNDOCUMENTED - refer to https://developer.calendly.com/api-docs for the exact timeout duration, whether partial responses are accepted, and how timeouts interact with the retry policy described in Webhook Errors]

---

## 5. CLI Decision

### Decision: Webhooks are excluded from calendly-cli

Webhooks are a **server-side subscription management** feature and are **not included** as CLI commands in calendly-cli.

### Rationale

Webhooks require a **publicly-accessible HTTP endpoint** to receive event payloads from Calendly's servers. This architectural requirement is fundamentally incompatible with the scope of a CLI tool:

1. **No persistent listener.** A CLI tool executes a command and exits. Webhooks require a long-running HTTP server to receive incoming POST requests at an unpredictable future time.

2. **No public endpoint.** CLI tools run on a developer's local machine, which is typically behind NAT, a firewall, or both. Calendly cannot deliver webhook events to `localhost` or a private IP address.

3. **Subscription management without reception is misleading.** While the CLI could theoretically create/list/delete webhook subscriptions via the API, doing so without the ability to receive the events would provide an incomplete and confusing user experience. Users would need to set up a separate HTTP server regardless.

4. **Scope alignment.** calendly-cli focuses on operations that produce immediate, synchronous results: querying user profiles, listing events, managing scheduling links. Webhooks are inherently asynchronous and event-driven.

### Future Consideration

If there is demand for webhook-adjacent CLI functionality, a **signature verification utility** subcommand could be added:

```
calendly webhook verify <payload> --secret <signing_secret>
```

This command would:

- Accept a raw webhook payload (from stdin or as an argument).
- Accept the signing secret and the signature header value.
- Compute the expected signature and compare it against the provided one.
- Output whether the signature is valid or invalid.

This would be useful for developers testing and debugging their webhook endpoint implementations without requiring the CLI to act as a webhook receiver.

---

## Notes

1. This document is a **reference-only** specification. No CLI commands correspond to webhooks.
2. The Calendly API does expose REST endpoints for managing webhook subscriptions (create, list, delete). These endpoints are intentionally not specified here because they are out of scope for this project. See https://developer.calendly.com/api-docs for the full webhook subscription API.
3. The raw Calendly API documentation available at the time of writing contained only stub descriptions for webhook signatures, errors, and timeouts. Sections above marked with `[UNDOCUMENTED]` should be updated if more detailed documentation becomes available.
