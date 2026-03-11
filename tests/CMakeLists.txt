set(TEST_SOURCES
    test_filter.cpp
    test_types.cpp
    test_models.cpp
    test_cache.cpp
)

# Need core source files for tests that use them
set(CORE_SOURCES
    ${CMAKE_SOURCE_DIR}/src/core/filter.cpp
    ${CMAKE_SOURCE_DIR}/src/core/cache.cpp
    ${CMAKE_SOURCE_DIR}/src/core/output.cpp
    ${CMAKE_SOURCE_DIR}/src/core/color.cpp
    ${CMAKE_SOURCE_DIR}/src/core/error.cpp
)

if(TEST_SOURCES)
    add_executable(calendly_tests ${TEST_SOURCES} ${CORE_SOURCES})

    target_include_directories(calendly_tests PRIVATE ${CMAKE_SOURCE_DIR}/src ${CMAKE_BINARY_DIR}/src)

    target_link_libraries(calendly_tests PRIVATE
        GTest::gtest_main
        nlohmann_json::nlohmann_json
        CLI11::CLI11
        tomlplusplus::tomlplusplus
        CURL::libcurl
    )

    target_compile_options(calendly_tests PRIVATE
        -Wall -Wextra -Wpedantic -Wshadow
    )

    include(GoogleTest)
    gtest_discover_tests(calendly_tests)
endif()
