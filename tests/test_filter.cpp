#include <gtest/gtest.h>
#include "core/filter.h"

// --- extract_uuid ---

TEST(FilterTest, ExtractUuidFromFullUri) {
    EXPECT_EQ(extract_uuid("https://api.calendly.com/users/AAAA-BBBB"),
              "AAAA-BBBB");
}

TEST(FilterTest, ExtractUuidFromNestedUri) {
    EXPECT_EQ(extract_uuid("https://api.calendly.com/event_types/abc-def-123"),
              "abc-def-123");
}

TEST(FilterTest, ExtractUuidFromBareString) {
    EXPECT_EQ(extract_uuid("no-slashes"), "no-slashes");
}

TEST(FilterTest, ExtractUuidFromEmptyString) {
    EXPECT_EQ(extract_uuid(""), "");
}

// --- build_uri ---

TEST(FilterTest, BuildUriUsers) {
    EXPECT_EQ(build_uri("users", "AAAA"),
              "https://api.calendly.com/users/AAAA");
}

TEST(FilterTest, BuildUriEventTypes) {
    EXPECT_EQ(build_uri("event_types", "1234-5678"),
              "https://api.calendly.com/event_types/1234-5678");
}

// --- is_valid_timezone ---

TEST(FilterTest, ValidTimezoneUtc) {
    EXPECT_TRUE(is_valid_timezone("UTC"));
}

TEST(FilterTest, ValidTimezoneIana) {
    EXPECT_TRUE(is_valid_timezone("America/New_York"));
    EXPECT_TRUE(is_valid_timezone("Europe/London"));
}

TEST(FilterTest, InvalidTimezoneEmpty) {
    EXPECT_FALSE(is_valid_timezone(""));
}

TEST(FilterTest, InvalidTimezoneNoSlash) {
    EXPECT_FALSE(is_valid_timezone("Eastern"));
}

// --- validate_count ---

TEST(FilterTest, ValidateCountNormal) {
    EXPECT_EQ(validate_count(20), 20);
    EXPECT_EQ(validate_count(1), 1);
    EXPECT_EQ(validate_count(100), 100);
}

TEST(FilterTest, ValidateCountClampLow) {
    EXPECT_EQ(validate_count(0), 1);
    EXPECT_EQ(validate_count(-5), 1);
}

TEST(FilterTest, ValidateCountClampHigh) {
    EXPECT_EQ(validate_count(101), 100);
    EXPECT_EQ(validate_count(999), 100);
}

// --- validate_sort ---

TEST(FilterTest, ValidateSortValid) {
    EXPECT_EQ(validate_sort("created_at:asc"), "created_at:asc");
    EXPECT_EQ(validate_sort("start_time:desc"), "start_time:desc");
}

TEST(FilterTest, ValidateSortInvalidFormat) {
    EXPECT_THROW(validate_sort("no_colon"), std::invalid_argument);
    EXPECT_THROW(validate_sort(":asc"), std::invalid_argument);
    EXPECT_THROW(validate_sort("field:"), std::invalid_argument);
}

TEST(FilterTest, ValidateSortInvalidDirection) {
    EXPECT_THROW(validate_sort("field:up"), std::invalid_argument);
    EXPECT_THROW(validate_sort("field:ASC"), std::invalid_argument);
}

// --- short_uuid ---

TEST(FilterTest, ShortUuidLong) {
    EXPECT_EQ(short_uuid("abcdefgh-1234-5678"), "abcdefgh");
}

TEST(FilterTest, ShortUuidShort) {
    EXPECT_EQ(short_uuid("abc"), "abc");
}

TEST(FilterTest, ShortUuidExact8) {
    EXPECT_EQ(short_uuid("12345678"), "12345678");
}

// --- strip_base_url ---

TEST(FilterTest, StripBaseUrlCalendly) {
    EXPECT_EQ(strip_base_url("https://api.calendly.com/users/AAAA"),
              "users/AAAA");
}

TEST(FilterTest, StripBaseUrlNonCalendly) {
    EXPECT_EQ(strip_base_url("https://other.com/stuff"),
              "https://other.com/stuff");
}

TEST(FilterTest, StripBaseUrlEmpty) {
    EXPECT_EQ(strip_base_url(""), "");
}

// --- format_relative_time ---

TEST(FilterTest, FormatRelativeTimeInvalidInput) {
    // Returns input unchanged if unparseable
    EXPECT_EQ(format_relative_time("not-a-date"), "not-a-date");
    EXPECT_EQ(format_relative_time(""), "");
}

// --- build_query_string ---

TEST(FilterTest, BuildQueryStringEmpty) {
    std::map<std::string, std::string> params;
    EXPECT_EQ(build_query_string(params), "");
}

TEST(FilterTest, BuildQueryStringSingle) {
    std::map<std::string, std::string> params{{"key", "value"}};
    EXPECT_EQ(build_query_string(params), "?key=value");
}

TEST(FilterTest, BuildQueryStringMultiple) {
    std::map<std::string, std::string> params{{"a", "1"}, {"b", "2"}};
    auto result = build_query_string(params);
    // std::map is ordered, so a comes before b
    EXPECT_EQ(result, "?a=1&b=2");
}

TEST(FilterTest, BuildQueryStringUrlEncoding) {
    std::map<std::string, std::string> params{{"q", "hello world"}};
    auto result = build_query_string(params);
    EXPECT_EQ(result, "?q=hello%20world");
}
