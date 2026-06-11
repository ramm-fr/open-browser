find_package(GTest REQUIRED)

set(TEST_SOURCES
    test_url_resolver.cpp
    test_bookmark_manager.cpp
    test_history_manager.cpp
    test_settings_manager.cpp
    test_ad_blocker.cpp
)

# One test executable containing all suites
add_executable(open-browser-tests ${TEST_SOURCES})

target_include_directories(open-browser-tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../src"
    "${CMAKE_CURRENT_BINARY_DIR}/.."
)

# Pull in the same sources as the main browser (exclude main.cpp and GUI code)
target_sources(open-browser-tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/url_resolver.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/settings_manager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/bookmark_manager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/history_manager.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/ad_blocker.cpp"
)

target_link_libraries(open-browser-tests PRIVATE
    GTest::GTest
    GTest::Main
    PkgConfig::GLIB
    PkgConfig::GIO
    PkgConfig::LIBSECRET
    nlohmann_json::nlohmann_json
)

# Register each test file with CTest
include(GoogleTest)
gtest_discover_tests(open-browser-tests)
