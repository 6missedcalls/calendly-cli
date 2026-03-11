#pragma once
#include <string>

namespace color {

[[nodiscard]] bool enabled() noexcept;
void set_enabled(bool enable) noexcept;

[[nodiscard]] std::string red(const std::string& s);
[[nodiscard]] std::string green(const std::string& s);
[[nodiscard]] std::string yellow(const std::string& s);
[[nodiscard]] std::string blue(const std::string& s);
[[nodiscard]] std::string magenta(const std::string& s);
[[nodiscard]] std::string cyan(const std::string& s);
[[nodiscard]] std::string gray(const std::string& s);
[[nodiscard]] std::string bold(const std::string& s);
[[nodiscard]] std::string dim(const std::string& s);
[[nodiscard]] std::string reset();

// Color from hex string (for event type colors like "#fff200")
std::string from_hex(const std::string& hex, const std::string& s);

// Status-aware coloring (Calendly-specific)
std::string event_status(const std::string& status, const std::string& s);      // confirmed=green, cancelled=red
std::string invitee_status(const std::string& status, const std::string& s);    // active=green, canceled=red, accepted=green, declined=yellow
std::string org_invitation_status(const std::string& status, const std::string& s); // pending=yellow, accepted=green, declined=red

}  // namespace color
