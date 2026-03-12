#include "color.h"

#include <cstdlib>
#include <sstream>
#include "platform.h"

namespace color {

namespace {

bool detect_color_support() {
    // NO_COLOR convention: https://no-color.org/
    if (std::getenv("NO_COLOR") != nullptr) {
        return false;
    }
    // Check if stdout is a terminal
    return isatty(STDOUT_FILENO) != 0;
}

bool& color_flag() {
    static bool flag = detect_color_support();
    return flag;
}

std::string wrap(const std::string& code, const std::string& s) {
    if (!enabled()) {
        return s;
    }
    return "\033[" + code + "m" + s + "\033[0m";
}

unsigned char hex_to_byte(const std::string& hex, size_t offset) {
    auto ch_to_val = [](char c) -> unsigned char {
        if (c >= '0' && c <= '9') return static_cast<unsigned char>(c - '0');
        if (c >= 'a' && c <= 'f') return static_cast<unsigned char>(c - 'a' + 10);
        if (c >= 'A' && c <= 'F') return static_cast<unsigned char>(c - 'A' + 10);
        return 0;
    };
    return static_cast<unsigned char>(ch_to_val(hex[offset]) * 16 + ch_to_val(hex[offset + 1]));
}

}  // namespace

bool enabled() noexcept {
    return color_flag();
}

void set_enabled(bool enable) noexcept {
    color_flag() = enable;
}

std::string red(const std::string& s) { return wrap("31", s); }
std::string green(const std::string& s) { return wrap("32", s); }
std::string yellow(const std::string& s) { return wrap("33", s); }
std::string cyan(const std::string& s) { return wrap("36", s); }
std::string gray(const std::string& s) { return wrap("90", s); }
std::string bold(const std::string& s) { return wrap("1", s); }
std::string dim(const std::string& s) { return wrap("2", s); }

std::string from_hex(const std::string& hex, const std::string& s) {
    if (!enabled()) {
        return s;
    }

    std::string h = hex;
    // Strip leading '#'
    if (!h.empty() && h[0] == '#') {
        h = h.substr(1);
    }

    if (h.size() != 6) {
        return s;
    }

    unsigned char r = hex_to_byte(h, 0);
    unsigned char g = hex_to_byte(h, 2);
    unsigned char b = hex_to_byte(h, 4);

    std::ostringstream oss;
    oss << "\033[38;2;" << static_cast<int>(r) << ";"
        << static_cast<int>(g) << ";" << static_cast<int>(b) << "m"
        << s << "\033[0m";
    return oss.str();
}

std::string event_status(const std::string& status, const std::string& s) {
    if (status == "confirmed" || status == "active") {
        return green(s);
    }
    if (status == "cancelled" || status == "canceled") {
        return red(s);
    }
    return s;
}

std::string invitee_status(const std::string& status, const std::string& s) {
    if (status == "active" || status == "accepted") {
        return green(s);
    }
    if (status == "canceled") {
        return red(s);
    }
    if (status == "declined") {
        return yellow(s);
    }
    return s;
}

std::string org_invitation_status(const std::string& status, const std::string& s) {
    if (status == "pending") {
        return yellow(s);
    }
    if (status == "accepted") {
        return green(s);
    }
    if (status == "declined") {
        return red(s);
    }
    return s;
}

}  // namespace color
