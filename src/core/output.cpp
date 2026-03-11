#include "output.h"
#include "color.h"

#include <algorithm>
#include <cstdlib>
#include <numeric>
#include <sstream>
#include "platform.h"

namespace {

OutputFormat g_format = OutputFormat::Table;
bool g_verbose = false;
int g_default_count = 20;

// Calculate visible length of a string, ignoring ANSI escape sequences
size_t visible_length(const std::string& s) {
    size_t len = 0;
    bool in_escape = false;
    for (size_t i = 0; i < s.size(); ++i) {
        if (in_escape) {
            if (s[i] == 'm') {
                in_escape = false;
            }
        } else if (s[i] == '\033' && i + 1 < s.size() && s[i + 1] == '[') {
            in_escape = true;
            ++i;  // skip '['
        } else {
            ++len;
        }
    }
    return len;
}

std::string csv_escape(const std::string& value) {
    bool needs_quoting = false;
    for (const char c : value) {
        if (c == '"' || c == ',' || c == '\n' || c == '\r') {
            needs_quoting = true;
            break;
        }
    }

    if (!needs_quoting) {
        return value;
    }

    std::string escaped = "\"";
    for (const char c : value) {
        if (c == '"') {
            escaped += "\"\"";
        } else {
            escaped += c;
        }
    }
    escaped += '"';
    return escaped;
}

void output_csv_values(const std::vector<std::string>& values, std::ostream& out) {
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) {
            out << ',';
        }
        out << csv_escape(values[i]);
    }
    out << '\n';
}

}  // namespace

OutputFormat get_output_format() noexcept {
    return g_format;
}

void set_output_format(OutputFormat fmt) noexcept {
    g_format = fmt;
}

// --- TableRenderer ---

TableRenderer::TableRenderer(const std::vector<TableColumn>& columns)
    : columns_(columns) {}

void TableRenderer::add_row(const std::vector<std::string>& values) {
    rows_.push_back(values);
}

bool TableRenderer::empty() const noexcept {
    return rows_.empty();
}

size_t TableRenderer::row_count() const noexcept {
    return rows_.size();
}

int TableRenderer::terminal_width() const {
    struct winsize w{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0) {
        return static_cast<int>(w.ws_col);
    }
    const char* cols_env = std::getenv("COLUMNS");
    if (cols_env != nullptr) {
        try {
            int cols = std::stoi(cols_env);
            if (cols > 0) {
                return cols;
            }
        } catch (...) {
            // Fall through to default
        }
    }
    return 120;
}

std::string TableRenderer::truncate(const std::string& s, int max_len) {
    if (max_len <= 0) {
        return "";
    }

    size_t vis_len = 0;
    size_t byte_pos = 0;
    bool in_escape = false;

    while (byte_pos < s.size()) {
        if (in_escape) {
            if (s[byte_pos] == 'm') {
                in_escape = false;
            }
            ++byte_pos;
        } else if (s[byte_pos] == '\033' && byte_pos + 1 < s.size() && s[byte_pos + 1] == '[') {
            in_escape = true;
            byte_pos += 2;
        } else {
            if (static_cast<int>(vis_len) >= max_len - 1) {
                // Find any trailing escape sequences to preserve reset
                std::string result = s.substr(0, byte_pos);
                // Check if the string had ANSI codes; if so, append reset
                if (s.find("\033[") != std::string::npos) {
                    result += "\033[0m";
                }
                // Replace last visible char with ellipsis indicator
                return result + "\xe2\x80\xa6";  // UTF-8 ellipsis
            }
            ++vis_len;
            ++byte_pos;
        }
    }

    return s;
}

std::string TableRenderer::pad(const std::string& s, int width, bool right_align) {
    int vis = static_cast<int>(visible_length(s));
    if (vis >= width) {
        return s;
    }
    int padding = width - vis;
    if (right_align) {
        return std::string(static_cast<size_t>(padding), ' ') + s;
    }
    return s + std::string(static_cast<size_t>(padding), ' ');
}

std::vector<int> TableRenderer::calculate_widths() const {
    size_t col_count = columns_.size();
    std::vector<int> widths(col_count);

    // Start with header widths
    for (size_t i = 0; i < col_count; ++i) {
        widths[i] = std::max(columns_[i].min_width,
                             static_cast<int>(columns_[i].header.size()));
    }

    // Expand to fit content
    for (const auto& row : rows_) {
        for (size_t i = 0; i < col_count && i < row.size(); ++i) {
            int content_width = static_cast<int>(visible_length(row[i]));
            widths[i] = std::max(widths[i], content_width);
        }
    }

    // Clamp to max_width
    for (size_t i = 0; i < col_count; ++i) {
        widths[i] = std::min(widths[i], columns_[i].max_width);
    }

    // Proportional shrinking if total exceeds terminal width
    int term_w = terminal_width();
    int separators = static_cast<int>(col_count) - 1;  // spaces between columns
    int total = std::accumulate(widths.begin(), widths.end(), 0) + separators * 2;

    if (total > term_w) {
        int excess = total - term_w;
        int shrinkable = 0;
        for (size_t i = 0; i < col_count; ++i) {
            if (widths[i] > columns_[i].min_width) {
                shrinkable += widths[i] - columns_[i].min_width;
            }
        }
        if (shrinkable > 0) {
            for (size_t i = 0; i < col_count && excess > 0; ++i) {
                int available = widths[i] - columns_[i].min_width;
                if (available > 0) {
                    int reduction = std::min(available, excess * available / shrinkable);
                    reduction = std::max(reduction, 1);
                    reduction = std::min(reduction, excess);
                    widths[i] -= reduction;
                    excess -= reduction;
                }
            }
        }
    }

    return widths;
}

void TableRenderer::render(std::ostream& out) const {
    if (columns_.empty()) {
        return;
    }

    auto widths = calculate_widths();
    size_t col_count = columns_.size();

    // Header
    for (size_t i = 0; i < col_count; ++i) {
        if (i > 0) {
            out << "  ";
        }
        std::string header = color::bold(columns_[i].header);
        out << pad(header, widths[i] + static_cast<int>(header.size() - columns_[i].header.size()),
                   columns_[i].right_align);
    }
    out << '\n';

    // Separator
    for (size_t i = 0; i < col_count; ++i) {
        if (i > 0) {
            out << "  ";
        }
        out << std::string(static_cast<size_t>(widths[i]), '-');
    }
    out << '\n';

    // Rows
    for (const auto& row : rows_) {
        for (size_t i = 0; i < col_count; ++i) {
            if (i > 0) {
                out << "  ";
            }
            std::string cell = (i < row.size()) ? row[i] : "";
            std::string truncated = truncate(cell, widths[i]);
            out << pad(truncated, widths[i], columns_[i].right_align);
        }
        out << '\n';
    }
}

// --- DetailRenderer ---

void DetailRenderer::add_field(const std::string& label, const std::string& value) {
    entries_.push_back({Entry::Field, label, value});
}

void DetailRenderer::add_field(const std::string& label, const std::optional<std::string>& value) {
    entries_.push_back({Entry::Field, label, value.value_or(color::dim("(none)"))});
}

void DetailRenderer::add_section(const std::string& title) {
    entries_.push_back({Entry::Section, title, ""});
}

void DetailRenderer::add_markdown(const std::string& content) {
    entries_.push_back({Entry::Markdown, "", content});
}

void DetailRenderer::add_blank_line() {
    entries_.push_back({Entry::Blank, "", ""});
}

void DetailRenderer::render(std::ostream& out) const {
    // Find max label width for alignment
    int max_label = 0;
    for (const auto& entry : entries_) {
        if (entry.type == Entry::Field) {
            max_label = std::max(max_label, static_cast<int>(entry.label.size()));
        }
    }

    for (const auto& entry : entries_) {
        switch (entry.type) {
            case Entry::Field: {
                int padding = max_label - static_cast<int>(entry.label.size());
                out << std::string(static_cast<size_t>(padding), ' ')
                    << color::bold(entry.label) << ": "
                    << entry.value << '\n';
                break;
            }
            case Entry::Section:
                out << '\n' << color::bold(color::cyan(entry.label)) << '\n';
                out << std::string(entry.label.size(), '-') << '\n';
                break;
            case Entry::Markdown:
                out << entry.value << '\n';
                break;
            case Entry::Blank:
                out << '\n';
                break;
        }
    }
}

// --- Free functions ---

void output_json(const nlohmann::json& data, std::ostream& out) {
    out << data.dump(2) << '\n';
}

void output_csv_row(const std::vector<std::string>& values, std::ostream& out) {
    output_csv_values(values, out);
}

void output_csv_header(const std::vector<std::string>& headers, std::ostream& out) {
    output_csv_values(headers, out);
}

void print_success(const std::string& message) {
    std::cout << color::green("\xe2\x9c\x93") << " " << message << '\n';
}

void print_warning(const std::string& message) {
    std::cerr << color::yellow("\xe2\x9a\xa0") << " " << message << '\n';
}

void print_error(const std::string& message) {
    std::cerr << color::red("\xe2\x9c\x97") << " " << message << '\n';
}

bool is_verbose() noexcept {
    return g_verbose;
}

void set_verbose(bool enabled) noexcept {
    g_verbose = enabled;
}

void print_verbose(const std::string& message) {
    if (g_verbose) {
        std::cerr << color::gray("[debug]") << " " << message << '\n';
    }
}

int get_default_count() noexcept {
    return g_default_count;
}

void set_default_count(int count) noexcept {
    g_default_count = count;
}
