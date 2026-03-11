cmake_minimum_required(VERSION 3.20)
project(calendly VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

configure_file(src/core/version.h.in src/core/version.h)

# --- FetchContent Dependencies ---
include(FetchContent)

FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)

FetchContent_Declare(cli11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG v2.4.2
)

FetchContent_Declare(tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG v3.4.0
)

FetchContent_MakeAvailable(json cli11 tomlplusplus)

if(BUILD_TESTING)
    FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.2
    )
    FetchContent_MakeAvailable(googletest)
endif()

# --- System Dependencies ---
find_package(CURL REQUIRED)

# --- Main Executable ---
add_executable(calendly
    src/main.cpp
    src/core/auth.cpp
    src/core/color.cpp
    src/core/config.cpp
    src/core/error.cpp
    src/core/filter.cpp
    src/core/http_client.cpp
    src/core/output.cpp
    src/core/rest_client.cpp
    src/core/cache.cpp
    src/core/resolver.cpp
    src/modules/users/api.cpp
    src/modules/users/commands.cpp
    src/modules/event_types/api.cpp
    src/modules/event_types/commands.cpp
    src/modules/events/api.cpp
    src/modules/events/commands.cpp
    src/modules/invitees/api.cpp
    src/modules/invitees/commands.cpp
    src/modules/organizations/api.cpp
    src/modules/organizations/commands.cpp
    src/modules/activity_log/api.cpp
    src/modules/activity_log/commands.cpp
    src/modules/one_off/api.cpp
    src/modules/one_off/commands.cpp
    src/modules/config/commands.cpp
    src/modules/cache/commands.cpp
    src/modules/book/commands.cpp
)

target_include_directories(calendly PRIVATE src ${CMAKE_BINARY_DIR}/src)

target_link_libraries(calendly PRIVATE
    nlohmann_json::nlohmann_json
    CLI11::CLI11
    tomlplusplus::tomlplusplus
    CURL::libcurl
)

target_compile_options(calendly PRIVATE
    -Wall -Wextra -Wpedantic -Wshadow
)

install(TARGETS calendly DESTINATION bin)

# --- Testing ---
option(BUILD_TESTING "Build tests" ON)
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()
