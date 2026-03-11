#include <gtest/gtest.h>
#include "modules/users/model.h"
#include "modules/event_types/model.h"

// --- User model ---

TEST(UserModelTest, FromJsonComplete) {
    auto j = nlohmann::json::parse(R"({
        "uri": "https://api.calendly.com/users/AAAA",
        "name": "Alice Example",
        "slug": "alice-example",
        "email": "alice@example.com",
        "scheduling_url": "https://calendly.com/alice-example",
        "timezone": "America/New_York",
        "time_notation": "12h",
        "avatar_url": "https://example.com/avatar.png",
        "created_at": "2024-01-01T00:00:00Z",
        "updated_at": "2024-06-01T00:00:00Z",
        "current_organization": "https://api.calendly.com/organizations/ORG1",
        "resource_type": "User",
        "locale": "en"
    })");

    User u = j.get<User>();
    EXPECT_EQ(u.uri, "https://api.calendly.com/users/AAAA");
    EXPECT_EQ(u.name, "Alice Example");
    EXPECT_EQ(u.slug, "alice-example");
    EXPECT_EQ(u.email, "alice@example.com");
    EXPECT_EQ(u.timezone, "America/New_York");
    EXPECT_EQ(u.time_notation, "12h");
    EXPECT_TRUE(u.avatar_url.has_value());
    EXPECT_EQ(u.avatar_url.value(), "https://example.com/avatar.png");
    EXPECT_EQ(u.current_organization, "https://api.calendly.com/organizations/ORG1");
    EXPECT_EQ(u.locale, "en");
}

TEST(UserModelTest, FromJsonNullableAvatar) {
    auto j = nlohmann::json::parse(R"({
        "uri": "https://api.calendly.com/users/BBBB",
        "name": "Test User",
        "avatar_url": null
    })");

    User u = j.get<User>();
    EXPECT_EQ(u.name, "Test User");
    EXPECT_FALSE(u.avatar_url.has_value());
}

TEST(UserModelTest, FromJsonMissingFields) {
    auto j = nlohmann::json::parse(R"({})");

    User u = j.get<User>();
    EXPECT_EQ(u.uri, "");
    EXPECT_EQ(u.name, "");
    EXPECT_EQ(u.time_notation, "12h");  // default
    EXPECT_EQ(u.resource_type, "User"); // default
    EXPECT_EQ(u.locale, "en");          // default
}

// --- EventType model ---

TEST(EventTypeModelTest, FromJsonComplete) {
    auto j = nlohmann::json::parse(R"({
        "uri": "https://api.calendly.com/event_types/ET1",
        "name": "30 Minute Meeting",
        "active": true,
        "slug": "30-minute-meeting",
        "scheduling_url": "https://calendly.com/alice/30-minute-meeting",
        "duration": 30,
        "kind": "solo",
        "type": "EventType",
        "color": "#0069FF",
        "created_at": "2024-01-01T00:00:00Z",
        "updated_at": "2024-06-01T00:00:00Z",
        "profile": {
            "type": "User",
            "name": "Alice Example",
            "owner": "https://api.calendly.com/users/AAAA"
        },
        "secret": false,
        "admin_managed": false,
        "custom_questions": [],
        "duration_options": [15, 30, 60],
        "locations": [
            {
                "kind": "zoom_conference"
            }
        ]
    })");

    EventType et = j.get<EventType>();
    EXPECT_EQ(et.uri, "https://api.calendly.com/event_types/ET1");
    EXPECT_EQ(et.name, "30 Minute Meeting");
    EXPECT_TRUE(et.active);
    EXPECT_EQ(et.slug, "30-minute-meeting");
    EXPECT_EQ(et.duration, 30);
    EXPECT_EQ(et.kind, "solo");
    EXPECT_EQ(et.profile.type, "User");
    EXPECT_EQ(et.profile.name, "Alice Example");
    EXPECT_FALSE(et.secret);
    EXPECT_EQ(et.custom_questions.size(), 0u);
    EXPECT_EQ(et.duration_options.size(), 3u);
    EXPECT_EQ(et.duration_options[1], 30);
    EXPECT_EQ(et.locations.size(), 1u);
    EXPECT_EQ(et.locations[0].kind, "zoom_conference");
}

TEST(EventTypeModelTest, FromJsonNullableFields) {
    auto j = nlohmann::json::parse(R"({
        "uri": "https://api.calendly.com/event_types/ET2",
        "name": "Quick Chat",
        "booking_method": null,
        "pooling_type": null,
        "internal_note": null,
        "description_plain": null,
        "description_html": null,
        "deleted_at": null,
        "is_paid": null
    })");

    EventType et = j.get<EventType>();
    EXPECT_FALSE(et.booking_method.has_value());
    EXPECT_FALSE(et.pooling_type.has_value());
    EXPECT_FALSE(et.internal_note.has_value());
    EXPECT_FALSE(et.description_plain.has_value());
    EXPECT_FALSE(et.description_html.has_value());
    EXPECT_FALSE(et.deleted_at.has_value());
    EXPECT_FALSE(et.is_paid.has_value());
}

TEST(EventTypeModelTest, CustomQuestionsFromJson) {
    auto j = nlohmann::json::parse(R"({
        "uri": "",
        "name": "",
        "custom_questions": [
            {
                "name": "Company",
                "type": "text",
                "position": 0,
                "enabled": true,
                "required": true,
                "answer_choices": [],
                "include_other": false
            },
            {
                "name": "Role",
                "type": "dropdown",
                "position": 1,
                "enabled": true,
                "required": false,
                "answer_choices": ["Engineering", "Sales", "Marketing"],
                "include_other": true
            }
        ]
    })");

    EventType et = j.get<EventType>();
    ASSERT_EQ(et.custom_questions.size(), 2u);
    EXPECT_EQ(et.custom_questions[0].name, "Company");
    EXPECT_EQ(et.custom_questions[0].type, "text");
    EXPECT_TRUE(et.custom_questions[0].required);
    EXPECT_EQ(et.custom_questions[1].answer_choices.size(), 3u);
    EXPECT_TRUE(et.custom_questions[1].include_other);
}

// --- AvailableTime model ---

TEST(AvailableTimeModelTest, FromJsonBasic) {
    auto j = nlohmann::json::parse(R"({
        "status": "available",
        "start_time": "2024-06-15T10:00:00Z",
        "invitees_remaining": 1
    })");

    AvailableTime at = j.get<AvailableTime>();
    EXPECT_EQ(at.status, "available");
    EXPECT_EQ(at.start_time, "2024-06-15T10:00:00Z");
    EXPECT_EQ(at.invitees_remaining, 1);
}

// --- Location model ---

TEST(LocationModelTest, FromJsonWithAllFields) {
    auto j = nlohmann::json::parse(R"({
        "kind": "physical",
        "location": "123 Main St",
        "additional_info": "Suite 100",
        "phone_number": "+1234567890"
    })");

    Location l = j.get<Location>();
    EXPECT_EQ(l.kind, "physical");
    EXPECT_TRUE(l.location.has_value());
    EXPECT_EQ(l.location.value(), "123 Main St");
    EXPECT_TRUE(l.additional_info.has_value());
    EXPECT_TRUE(l.phone_number.has_value());
}

TEST(LocationModelTest, FromJsonMinimal) {
    auto j = nlohmann::json::parse(R"({"kind": "zoom_conference"})");

    Location l = j.get<Location>();
    EXPECT_EQ(l.kind, "zoom_conference");
    EXPECT_FALSE(l.location.has_value());
    EXPECT_FALSE(l.additional_info.has_value());
    EXPECT_FALSE(l.phone_number.has_value());
}
