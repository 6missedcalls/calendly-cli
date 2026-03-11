#pragma once
#include <map>
#include <optional>
#include <string>

// Extract UUID from full Calendly URI
// "https://api.calendly.com/users/AAAA..." -> "AAAA..."
std::string extract_uuid(const std::string& uri);

// Build full URI from resource type and UUID
// ("users", "AAAA...") -> "https://api.calendly.com/users/AAAA..."
std::string build_uri(const std::string& resource_type, const std::string& uuid);

// Validate IANA timezone string (basic validation - check format, not exhaustive list)
bool is_valid_timezone(const std::string& tz);

// Build query parameter string from map, URL-encoding values
std::string build_query_string(const std::map<std::string, std::string>& params);

// Parse ISO 8601 timestamp for display
std::string format_relative_time(const std::string& iso8601);
std::string format_local_time(const std::string& iso8601, const std::string& timezone);

// Validate count parameter (Resolution H-06): 1 <= count <= 100
int validate_count(int count);

// Sort parameter validation (Resolution C-04): ensures "field:direction" format
std::string validate_sort(const std::string& sort);

// Shorten UUID for table display: first 8 characters
std::string short_uuid(const std::string& uuid);

// Strip Calendly base URL from URIs for table display
std::string strip_base_url(const std::string& uri);
