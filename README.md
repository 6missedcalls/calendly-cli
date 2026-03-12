<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-20-blue?style=flat-square&logo=cplusplus" alt="C++20">
  <img src="https://img.shields.io/badge/platform-macOS%20%7C%20Linux-lightgrey?style=flat-square" alt="Platform">
  <img src="https://img.shields.io/badge/license-MIT-green?style=flat-square" alt="License">
</p>

<pre align="center">
   ██████╗ █████╗ ██╗     ███████╗███╗   ██╗██████╗ ██╗  ██╗   ██╗
  ██╔════╝██╔══██╗██║     ██╔════╝████╗  ██║██╔══██╗██║  ╚██╗ ██╔╝
  ██║     ███████║██║     █████╗  ██╔██╗ ██║██║  ██║██║   ╚████╔╝
  ██║     ██╔══██║██║     ██╔══╝  ██║╚██╗██║██║  ██║██║    ╚██╔╝
  ╚██████╗██║  ██║███████╗███████╗██║ ╚████║██████╔╝███████╗██║
   ╚═════╝╚═╝  ╚═╝╚══════╝╚══════╝╚═╝  ╚═══╝╚═════╝ ╚══════╝╚═╝
</pre>

<h3 align="center">Fast, single-binary CLI for <a href="https://calendly.com">Calendly</a></h3>

<p align="center">
  Full <a href="https://developer.calendly.com/api-docs">Calendly API</a> access from your terminal.<br>
  9 modules &middot; 15+ endpoints &middot; built for humans and AI agents alike.
</p>

---

## Why calendly?

[Calendly](https://calendly.com) is a scheduling platform used by millions. **calendly** brings the full Calendly workflow to your terminal -- no browser needed.

- **Fast** -- native C++ binary, sub-100ms response times, zero runtime overhead
- **Complete** -- events, event types, invitees, organizations, activity log, booking, and more
- **AI-friendly** -- every command supports `--json` for structured output, purpose-built for LLM tool use
- **Smart** -- name-to-URI resolver lets you reference event types by name, slug, UUID prefix, or full URI
- **Portable** -- single binary, no runtime dependencies

```
$ calendly events list --today
STATUS     NAME                       START TIME           END TIME             INVITEES
---------  -------------------------  -------------------  -------------------  --------
confirmed  30 Minute Meeting          2026-03-11 10:00 AM  2026-03-11 10:30 AM  1
confirmed  Product Demo               2026-03-11 02:00 PM  2026-03-11 03:00 PM  3
```

## Getting started

### 1. Install

**Build from source**:

```sh
# install prerequisites
# macOS:  brew install cmake curl
# Ubuntu: sudo apt install cmake libcurl4-openssl-dev build-essential

git clone https://github.com/6missedcalls/calendly-cli.git
cd calendly-cli
make install    # builds + installs to /usr/local/bin
```

All library dependencies (CLI11, nlohmann/json, toml++) are fetched automatically by CMake.

### 2. Authenticate

Get your Personal Access Token from [Calendly Integrations > API](https://calendly.com/integrations/api_webhooks), then:

```sh
# Option A: environment variable (add to ~/.zshrc or ~/.bashrc to persist)
export CALENDLY_API_TOKEN="your_token_here"

# Option B: save to config file
calendly config set token "your_token_here"
```

### 3. Verify

```sh
$ calendly me
      Name: Alice Chen
     Email: alice@example.com
  Timezone: America/New_York
```

You're ready. Run `calendly --help` to see all commands, or keep reading.

## Usage

```
calendly <command> [subcommand] [flags]
```

Global flags (`--json`, `--csv`, `--no-color`, `--no-cache`, `--verbose`) work in any position:

```sh
calendly events list --json          # structured JSON output
calendly --json events list          # same thing
calendly events list --no-cache      # bypass cache
```

### Command overview

| Group | Commands | Description |
|-------|----------|-------------|
| **Scheduling** | `events` `book` `one-off` | List, show, cancel, and book events |
| **Discovery** | `event-types` `invitees` | Browse event types, time slots, and invitees |
| **Workspace** | `org` `activity-log` `me` | Organization, audit log, and user profile |
| **System** | `config` `cache` | CLI configuration and local cache |

Run any command bare to see its subcommands:

```sh
calendly events          # shows all event subcommands
calendly event-types     # shows all event-type subcommands
```

### Common workflows

```sh
# Browse events
calendly events list
calendly events list --today --status confirmed
calendly events list --this-week --json
calendly events show <uuid>

# Book a meeting
calendly book --type "30 Minute Meeting" --name "Jane" --email "jane@example.com" --first
calendly book --type "30min" --name "Jane" --email "jane@example.com" --dry-run

# Cancel an event
calendly events cancel <uuid> --reason "conflict" --yes

# Event types and availability
calendly event-types list
calendly event-types available-times --type "30 Minute Meeting" --tomorrow

# Invitees
calendly invitees list <event-uuid>

# Organization
calendly org invitations

# Activity log
calendly activity-log list

# One-off event types
calendly one-off create --name "Quick Chat" --duration 15
```

### JSON output for AI agents

Every command supports `--json`, producing structured output for LLM tool use and scripting:

```sh
$ calendly me --json
{
  "uri": "https://api.calendly.com/users/XXXXXXXX",
  "name": "Alice Chen",
  "email": "alice@example.com",
  "timezone": "America/New_York"
}
```

Non-interactive mode for automation:

```sh
# --json for structured output, --yes/--first to skip prompts
calendly events list --json --today
calendly events cancel <uuid> --yes --json
calendly book --type "Meeting" --name "Bot" --email "bot@example.com" --first --json
```

## Configuration

```sh
calendly config set token "your_token_here"
calendly config set timezone "America/Chicago"
calendly config set color false
calendly config set count 50
calendly config show
```

Stored at `~/.config/calendly/config.toml`:

| Key | Values | Default |
|-----|--------|---------|
| `token` | Calendly PAT | -- |
| `timezone` | IANA timezone | from user profile |
| `color` | `true` `false` | `true` |
| `count` | `1` -- `100` | `20` |

## Architecture

```
src/
├── main.cpp                  Entry point, CLI registration
├── core/
│   ├── http_client.{h,cpp}   libcurl wrapper, TLS, rate-limit headers
│   ├── rest_client.{h,cpp}   REST API methods (GET, POST)
│   ├── auth.{h,cpp}          PAT resolution (env var / config file)
│   ├── config.{h,cpp}        TOML config read/write
│   ├── output.{h,cpp}        Table renderer, detail renderer, CSV, JSON
│   ├── color.{h,cpp}         ANSI color, pipe detection, hex-to-RGB
│   ├── error.{h,cpp}         Typed errors, retry-after, rate limiting
│   ├── filter.{h,cpp}        UUID extraction, timezone validation, helpers
│   ├── paginator.h            Cursor-based pagination
│   ├── resolver.{h,cpp}      Name/slug/UUID → full URI resolution
│   ├── cache.{h,cpp}         JSON file cache with TTLs
│   ├── platform.h             Portability header (Unix APIs)
│   └── types.h               Shared types, enums, JSON converters
└── modules/
    └── <name>/               One directory per domain
        ├── model.h            Data structs + JSON deserialization
        ├── api.{h,cpp}        REST API calls
        └── commands.{h,cpp}   CLI11 command registration + handlers
```

Each module follows a consistent three-file pattern: **model** (data) -> **api** (network) -> **commands** (UI). Core infrastructure is shared across all modules.

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the full architecture document.

## Development

```sh
make build       # release build (optimized)
make debug       # debug build (symbols, sanitizers)
make test        # build + run 67 tests via CTest
make clean       # remove build artifacts
make format      # clang-format all source files
make lint        # clang-tidy static analysis
```

### Tech stack

| Layer | Technology |
|-------|-----------|
| Language | C++20 (clang++) |
| Build | CMake 3.20+ |
| HTTP | [libcurl](https://curl.se/libcurl/) |
| JSON | [nlohmann/json](https://github.com/nlohmann/json) v3.11.3 |
| CLI | [CLI11](https://github.com/CLIUtils/CLI11) v2.4.2 |
| Config | [toml++](https://github.com/marzer/tomlplusplus) v3.4.0 |
| Tests | [GoogleTest](https://github.com/google/googletest) v1.15.2 |

### Running tests

```sh
make test
# or directly:
cd build && ctest --output-on-failure
```

The test suite covers JSON parsing, model deserialization, filter utilities, cache operations, and type conversions. All tests run offline -- no API key required.

## License

[MIT](LICENSE)

---

<p align="center">
  Built by <a href="https://github.com/6missedcalls">6missedcalls</a>
</p>
