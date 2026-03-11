# calendly-cli

A fast, single-binary CLI for the [Calendly REST API](https://developer.calendly.com/api-docs). Built for humans and AI agents.

```
   ██████╗ █████╗ ██╗     ███████╗███╗   ██╗██████╗ ██╗  ██╗   ██╗
  ██╔════╝██╔══██╗██║     ██╔════╝████╗  ██║██╔══██╗██║  ╚██╗ ██╔╝
  ██║     ███████║██║     █████╗  ██╔██╗ ██║██║  ██║██║   ╚████╔╝
  ██║     ██╔══██║██║     ██╔══╝  ██║╚██╗██║██║  ██║██║    ╚██╔╝
  ╚██████╗██║  ██║███████╗███████╗██║ ╚████║██████╔╝███████╗██║
   ╚═════╝╚═╝  ╚═╝╚══════╝╚══════╝╚═╝  ╚═══╝╚═════╝ ╚══════╝╚═╝
```

## Features

- List, show, cancel, and book events
- Browse event types and available time slots
- List invitees, org invitations, and activity logs
- Create one-off event types
- Smart resolver: reference event types by name, slug, UUID prefix, or full URI
- JSON file cache with TTLs (user 24h, event types 1h)
- Output formats: table (default), `--json`, `--csv`
- Global flags: `--no-color`, `--no-cache`, `--verbose`
- Non-interactive mode via `--first` and `--yes` flags for AI agent usage

## Requirements

- C++20 compiler (GCC 11+, Clang 14+, Apple Clang 15+)
- CMake 3.20+
- libcurl (system)

## Build

```bash
make build          # Release build
make debug          # Debug build
make test           # Run tests
make install        # Install to /usr/local/bin
```

Or manually:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/calendly --version
```

## Setup

Get a [Calendly Personal Access Token](https://calendly.com/integrations/api_webhooks) and configure it:

```bash
# Option 1: Environment variable
export CALENDLY_API_TOKEN=your_token_here

# Option 2: Config file
./build/calendly config set token your_token_here
```

## Usage

```bash
# Your profile
calendly me
calendly me --json

# Events
calendly events list --today
calendly events list --this-week --status confirmed
calendly events show <uuid>
calendly events cancel <uuid> --reason "conflict" --yes

# Event types
calendly event-types list
calendly event-types available-times --type "30 Minute Meeting" --tomorrow

# Book an event
calendly book --type "30 Minute Meeting" --name "Jane Doe" --email "jane@example.com" --first
calendly book --type "30 Minute Meeting" --name "Jane Doe" --email "jane@example.com" --dry-run

# Invitees
calendly invitees list <event-uuid>

# Organization
calendly org invitations

# Activity log
calendly activity-log list

# One-off event types
calendly one-off create --name "Quick Chat" --duration 15

# Cache
calendly cache show
calendly cache clear

# Config
calendly config show
calendly config path
```

## AI Agent Usage

For non-interactive automation, use `--json` for structured output and `--yes`/`--first` to skip prompts:

```bash
calendly events list --json --today
calendly events cancel <uuid> --yes --json
calendly book --type "Meeting" --name "Bot" --email "bot@example.com" --first --json
```

## Architecture

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for detailed architecture documentation.

## License

[MIT](LICENSE)
