#include <gtest/gtest.h>
#include "core/types.h"

// Helper: compare enums by casting to int (avoids needing operator<< for GTest)
#define EXPECT_ENUM_EQ(a, b) EXPECT_EQ(static_cast<int>(a), static_cast<int>(b))

// --- CursorPageInfo ---

TEST(TypesTest, CursorPageInfoFromJson) {
    auto j = nlohmann::json::parse(R"({
        "count": 20,
        "next_page": "https://api.calendly.com/x?page_token=abc",
        "next_page_token": "abc"
    })");

    CursorPageInfo info = j.get<CursorPageInfo>();
    EXPECT_EQ(info.count, 20);
    EXPECT_TRUE(info.next_page.has_value());
    EXPECT_EQ(info.next_page.value(), "https://api.calendly.com/x?page_token=abc");
    EXPECT_TRUE(info.next_page_token.has_value());
    EXPECT_EQ(info.next_page_token.value(), "abc");
}

TEST(TypesTest, CursorPageInfoNullNextPage) {
    auto j = nlohmann::json::parse(R"({
        "count": 5,
        "next_page": null,
        "next_page_token": null
    })");

    CursorPageInfo info = j.get<CursorPageInfo>();
    EXPECT_EQ(info.count, 5);
    EXPECT_FALSE(info.next_page.has_value());
    EXPECT_FALSE(info.next_page_token.has_value());
}

TEST(TypesTest, CursorPageInfoMissingFields) {
    auto j = nlohmann::json::parse(R"({})");

    CursorPageInfo info = j.get<CursorPageInfo>();
    EXPECT_EQ(info.count, 0);
    EXPECT_FALSE(info.next_page.has_value());
    EXPECT_FALSE(info.next_page_token.has_value());
}

// --- Tracking ---

TEST(TypesTest, TrackingFromJson) {
    auto j = nlohmann::json::parse(R"({
        "utm_campaign": "spring",
        "utm_source": "google",
        "utm_medium": "cpc",
        "utm_content": "ad1",
        "utm_term": "calendly",
        "salesforce_uuid": "sf-123"
    })");

    Tracking t = j.get<Tracking>();
    EXPECT_EQ(t.utm_campaign.value(), "spring");
    EXPECT_EQ(t.utm_source.value(), "google");
    EXPECT_EQ(t.salesforce_uuid.value(), "sf-123");
}

TEST(TypesTest, TrackingEmptyJson) {
    auto j = nlohmann::json::parse(R"({})");

    Tracking t = j.get<Tracking>();
    EXPECT_FALSE(t.utm_campaign.has_value());
    EXPECT_FALSE(t.utm_source.has_value());
}

// --- QuestionAndAnswer ---

TEST(TypesTest, QuestionAndAnswerFromJson) {
    auto j = nlohmann::json::parse(R"({
        "answer": "Blue",
        "position": 0,
        "question": "Favorite color?"
    })");

    QuestionAndAnswer qa = j.get<QuestionAndAnswer>();
    EXPECT_EQ(qa.question, "Favorite color?");
    EXPECT_EQ(qa.answer, "Blue");
    EXPECT_EQ(qa.position, 0);
}

// --- Cancellation ---

TEST(TypesTest, CancellationFromJson) {
    auto j = nlohmann::json::parse(R"({
        "canceled_by": "Alice Example",
        "reason": "Schedule conflict"
    })");

    Cancellation c = j.get<Cancellation>();
    EXPECT_TRUE(c.canceled_by.has_value());
    EXPECT_EQ(c.canceled_by.value(), "Alice Example");
    EXPECT_EQ(c.reason.value(), "Schedule conflict");
}

TEST(TypesTest, CancellationFromJsonNullFields) {
    auto j = nlohmann::json::parse(R"({
        "canceled_by": null,
        "reason": null
    })");

    Cancellation c = j.get<Cancellation>();
    EXPECT_FALSE(c.canceled_by.has_value());
    EXPECT_FALSE(c.reason.has_value());
}

// --- EventStatus parsing ---

TEST(TypesTest, EventStatusFromStringValid) {
    EXPECT_ENUM_EQ(parse_event_status("confirmed"), EventStatus::Confirmed);
    EXPECT_ENUM_EQ(parse_event_status("cancelled"), EventStatus::Cancelled);
    EXPECT_ENUM_EQ(parse_event_status("canceled"), EventStatus::Cancelled);  // alias
}

TEST(TypesTest, EventStatusFromStringCaseInsensitive) {
    EXPECT_ENUM_EQ(parse_event_status("CONFIRMED"), EventStatus::Confirmed);
    EXPECT_ENUM_EQ(parse_event_status("Confirmed"), EventStatus::Confirmed);
    EXPECT_ENUM_EQ(parse_event_status("Cancelled"), EventStatus::Cancelled);
}

TEST(TypesTest, EventStatusFromStringUnknown) {
    EXPECT_ENUM_EQ(parse_event_status("pending"), EventStatus::Unknown);
    EXPECT_ENUM_EQ(parse_event_status("active"), EventStatus::Unknown);
    EXPECT_ENUM_EQ(parse_event_status(""), EventStatus::Unknown);
}

// --- InviteeStatus parsing ---

TEST(TypesTest, InviteeStatusFromStringValid) {
    EXPECT_ENUM_EQ(parse_invitee_status("active"), InviteeStatus::Active);
    EXPECT_ENUM_EQ(parse_invitee_status("canceled"), InviteeStatus::Canceled);
    EXPECT_ENUM_EQ(parse_invitee_status("cancelled"), InviteeStatus::Canceled);  // alias
    EXPECT_ENUM_EQ(parse_invitee_status("accepted"), InviteeStatus::Accepted);
    EXPECT_ENUM_EQ(parse_invitee_status("declined"), InviteeStatus::Declined);
}

TEST(TypesTest, InviteeStatusFromStringCaseInsensitive) {
    EXPECT_ENUM_EQ(parse_invitee_status("ACTIVE"), InviteeStatus::Active);
    EXPECT_ENUM_EQ(parse_invitee_status("Canceled"), InviteeStatus::Canceled);
}

TEST(TypesTest, InviteeStatusFromStringUnknown) {
    EXPECT_ENUM_EQ(parse_invitee_status(""), InviteeStatus::Unknown);
    EXPECT_ENUM_EQ(parse_invitee_status("expired"), InviteeStatus::Unknown);
}

// --- LocationKind parsing ---

TEST(TypesTest, LocationKindFromStringValid) {
    EXPECT_ENUM_EQ(parse_location_kind("physical"), LocationKind::Physical);
    EXPECT_ENUM_EQ(parse_location_kind("google_conference"), LocationKind::GoogleConference);
    EXPECT_ENUM_EQ(parse_location_kind("zoom_conference"), LocationKind::ZoomConference);
    EXPECT_ENUM_EQ(parse_location_kind("custom"), LocationKind::Custom);
    EXPECT_ENUM_EQ(parse_location_kind("ask_invitee"), LocationKind::AskInvitee);
    EXPECT_ENUM_EQ(parse_location_kind("outbound_call"), LocationKind::OutboundCall);
    EXPECT_ENUM_EQ(parse_location_kind("inbound_call"), LocationKind::InboundCall);
}

TEST(TypesTest, LocationKindFromStringAliases) {
    // Alias resolution (Resolution C-03)
    EXPECT_ENUM_EQ(parse_location_kind("zoom"), LocationKind::ZoomConference);
    EXPECT_ENUM_EQ(parse_location_kind("google_hangouts"), LocationKind::GoogleConference);
    EXPECT_ENUM_EQ(parse_location_kind("microsoft_teams"), LocationKind::MicrosoftTeamsConference);
    EXPECT_ENUM_EQ(parse_location_kind("phone_call"), LocationKind::OutboundCall);
}

TEST(TypesTest, LocationKindFromStringUnknown) {
    EXPECT_ENUM_EQ(parse_location_kind("teleportation"), LocationKind::Unknown);
    EXPECT_ENUM_EQ(parse_location_kind(""), LocationKind::Unknown);
}
