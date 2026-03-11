#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include "core/cache.h"

// Tests use a temporary XDG_CACHE_HOME to avoid polluting real cache

class CacheTest : public ::testing::Test {
protected:
    std::string original_xdg;
    std::string test_cache_dir;

    void SetUp() override {
        // Save and override XDG_CACHE_HOME
        const char* xdg = std::getenv("XDG_CACHE_HOME");
        original_xdg = xdg ? xdg : "";

        test_cache_dir = std::filesystem::temp_directory_path().string() +
                         "/calendly-test-cache-" + std::to_string(getpid());
        setenv("XDG_CACHE_HOME", test_cache_dir.c_str(), 1);

        // Ensure no-cache is disabled for tests
        set_cache_disabled(false);
    }

    void TearDown() override {
        // Clean up
        std::error_code ec;
        std::filesystem::remove_all(test_cache_dir, ec);

        if (original_xdg.empty()) {
            unsetenv("XDG_CACHE_HOME");
        } else {
            setenv("XDG_CACHE_HOME", original_xdg.c_str(), 1);
        }
    }

    User make_test_user() {
        User u;
        u.uri = "https://api.calendly.com/users/TEST-UUID";
        u.name = "Test User";
        u.slug = "test-user";
        u.email = "test@example.com";
        u.scheduling_url = "https://calendly.com/test-user";
        u.timezone = "America/Chicago";
        u.time_notation = "12h";
        u.created_at = "2024-01-01T00:00:00Z";
        u.updated_at = "2024-06-01T00:00:00Z";
        u.current_organization = "https://api.calendly.com/organizations/ORG1";
        u.resource_type = "User";
        u.locale = "en";
        return u;
    }

    EventType make_test_event_type(const std::string& name, const std::string& uuid) {
        EventType et;
        et.uri = "https://api.calendly.com/event_types/" + uuid;
        et.name = name;
        et.active = true;
        et.slug = name;  // simplified
        et.scheduling_url = "https://calendly.com/test/" + name;
        et.duration = 30;
        et.kind = "solo";
        et.type = "EventType";
        et.color = "#0069FF";
        et.created_at = "2024-01-01T00:00:00Z";
        et.updated_at = "2024-06-01T00:00:00Z";
        et.profile = {"User", "Test User", "https://api.calendly.com/users/TEST"};
        return et;
    }
};

// --- Basic cache operations ---

TEST_F(CacheTest, EmptyCacheReturnsNullopt) {
    Cache cache;
    EXPECT_FALSE(cache.get_user().has_value());
    EXPECT_FALSE(cache.get_event_types().has_value());
}

TEST_F(CacheTest, SetAndGetUser) {
    Cache cache;
    auto user = make_test_user();
    cache.set_user(user);

    auto result = cache.get_user();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->name, "Test User");
    EXPECT_EQ(result->email, "test@example.com");
    EXPECT_EQ(result->timezone, "America/Chicago");
    EXPECT_EQ(result->uri, "https://api.calendly.com/users/TEST-UUID");
}

TEST_F(CacheTest, SetAndGetEventTypes) {
    Cache cache;
    std::vector<EventType> types = {
        make_test_event_type("30min", "ET-1"),
        make_test_event_type("60min", "ET-2")
    };
    cache.set_event_types(types);

    auto result = cache.get_event_types();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->size(), 2u);
    EXPECT_EQ((*result)[0].name, "30min");
    EXPECT_EQ((*result)[1].name, "60min");
}

// --- Cache invalidation ---

TEST_F(CacheTest, ClearAllRemovesEverything) {
    Cache cache;
    cache.set_user(make_test_user());
    cache.set_event_types({make_test_event_type("test", "ET-1")});

    cache.clear_all();

    EXPECT_FALSE(cache.get_user().has_value());
    EXPECT_FALSE(cache.get_event_types().has_value());
}

TEST_F(CacheTest, ClearUserOnlyAffectsUser) {
    Cache cache;
    cache.set_user(make_test_user());
    cache.set_event_types({make_test_event_type("test", "ET-1")});

    cache.clear_user();

    EXPECT_FALSE(cache.get_user().has_value());
    EXPECT_TRUE(cache.get_event_types().has_value());
}

TEST_F(CacheTest, ClearEventTypesOnlyAffectsEventTypes) {
    Cache cache;
    cache.set_user(make_test_user());
    cache.set_event_types({make_test_event_type("test", "ET-1")});

    cache.clear_event_types();

    EXPECT_TRUE(cache.get_user().has_value());
    EXPECT_FALSE(cache.get_event_types().has_value());
}

// --- No-cache bypass ---

TEST_F(CacheTest, NoCacheBypasses) {
    Cache cache;
    cache.set_user(make_test_user());

    set_cache_disabled(true);
    EXPECT_FALSE(cache.get_user().has_value());

    set_cache_disabled(false);
    EXPECT_TRUE(cache.get_user().has_value());
}

// --- Cache freshness ---

TEST_F(CacheTest, IsFreshReturnsTrueForNewEntry) {
    Cache cache;
    cache.set_user(make_test_user());
    EXPECT_TRUE(cache.is_fresh("user"));
}

TEST_F(CacheTest, IsFreshReturnsFalseForMissing) {
    Cache cache;
    EXPECT_FALSE(cache.is_fresh("user"));
    EXPECT_FALSE(cache.is_fresh("nonexistent"));
}

// --- Status ---

TEST_F(CacheTest, StatusShowsEmptyCache) {
    Cache cache;
    auto s = cache.status();
    EXPECT_FALSE(s.user_cached);
    EXPECT_FALSE(s.event_types_cached);
    EXPECT_EQ(s.event_types_count, 0);
}

TEST_F(CacheTest, StatusShowsPopulatedCache) {
    Cache cache;
    cache.set_user(make_test_user());
    cache.set_event_types({
        make_test_event_type("a", "1"),
        make_test_event_type("b", "2"),
        make_test_event_type("c", "3")
    });

    auto s = cache.status();
    EXPECT_TRUE(s.user_cached);
    EXPECT_TRUE(s.event_types_cached);
    EXPECT_EQ(s.event_types_count, 3);
    EXPECT_FALSE(s.user_cached_at.empty());
    EXPECT_FALSE(s.event_types_cached_at.empty());
}

// --- Cache persistence across instances ---

TEST_F(CacheTest, PersistsAcrossInstances) {
    {
        Cache cache1;
        cache1.set_user(make_test_user());
    }
    {
        Cache cache2;
        auto result = cache2.get_user();
        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(result->name, "Test User");
    }
}

// --- Edge cases ---

TEST_F(CacheTest, ClearOnEmptyCacheDoesNotError) {
    Cache cache;
    EXPECT_NO_THROW(cache.clear_all());
    EXPECT_NO_THROW(cache.clear_user());
    EXPECT_NO_THROW(cache.clear_event_types());
}

TEST_F(CacheTest, CacheDir) {
    Cache cache;
    EXPECT_FALSE(cache.cache_dir().empty());
    EXPECT_TRUE(cache.cache_dir().find("calendly") != std::string::npos);
}
