cmake_minimum_required(VERSION 3.20)

project(open-browser
    VERSION 1.1.0
    DESCRIPTION "A modern, minimalist, privacy-focused web browser for Linux"
    LANGUAGES CXX
)

# C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDE/LSP support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wno-unused-parameter
        $<$<CONFIG:Release>:-O2>
        $<$<CONFIG:Debug>:-g>
        $<$<CONFIG:Debug>:-O0>
    )
endif()

# Find required packages
find_package(PkgConfig REQUIRED)

pkg_check_modules(GTK4 REQUIRED IMPORTED_TARGET gtk4>=4.6)
# Try webkitgtk-6.0 first, fall back to webkit2gtk-4.1
pkg_check_modules(WEBKIT6 IMPORTED_TARGET webkitgtk-6.0>=2.40)
if(NOT WEBKIT6_FOUND)
    pkg_check_modules(WEBKIT REQUIRED IMPORTED_TARGET webkit2gtk-4.1>=2.38)
    add_definitions(-DUSE_WEBKIT2GTK)
else()
    # Create an alias so src/CMakeLists.txt can use PkgConfig::WEBKIT
    add_library(PkgConfig::WEBKIT ALIAS PkgConfig::WEBKIT6)
endif()
pkg_check_modules(LIBSOUP REQUIRED IMPORTED_TARGET libsoup-3.0)
pkg_check_modules(GLIB REQUIRED IMPORTED_TARGET glib-2.0)
pkg_check_modules(GIO REQUIRED IMPORTED_TARGET gio-2.0)
pkg_check_modules(LIBSECRET REQUIRED IMPORTED_TARGET libsecret-1)

# nlohmann/json — header-only, find via pkg-config or fallback to bundled
find_package(nlohmann_json 3.10 QUIET)
if(NOT nlohmann_json_FOUND)
    pkg_check_modules(NLOHMANN_JSON QUIET nlohmann_json)
    if(NOT NLOHMANN_JSON_FOUND)
        # Try system include path directly
        find_path(NLOHMANN_JSON_INCLUDE_DIR
            NAMES nlohmann/json.hpp
            PATHS /usr/include /usr/local/include
        )
        if(NLOHMANN_JSON_INCLUDE_DIR)
            message(STATUS "Found nlohmann/json at ${NLOHMANN_JSON_INCLUDE_DIR}")
            add_library(nlohmann_json::nlohmann_json INTERFACE IMPORTED)
            target_include_directories(nlohmann_json::nlohmann_json
                INTERFACE ${NLOHMANN_JSON_INCLUDE_DIR}
            )
        else()
            message(FATAL_ERROR "nlohmann/json not found. Install nlohmann-json3-dev (Ubuntu) or nlohmann-json-devel (Fedora).")
        endif()
    else()
        add_library(nlohmann_json::nlohmann_json INTERFACE IMPORTED)
        target_include_directories(nlohmann_json::nlohmann_json
            INTERFACE ${NLOHMANN_JSON_INCLUDE_DIRS}
        )
    endif()
endif()

# Version info header
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/version.h"
)

# Include generated headers
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

# Main source
add_subdirectory(src)

# Tests
option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    find_package(GTest QUIET)
    if(GTest_FOUND)
        enable_testing()
        add_subdirectory(tests)
    else()
        message(STATUS "GTest not found, skipping tests")
    endif()
endif()

# Install desktop entry
install(
    FILES resources/io.openbrowser.Browser.desktop
    DESTINATION share/applications
)

# Install metainfo
install(
    FILES resources/io.openbrowser.Browser.metainfo.xml
    DESTINATION share/metainfo
)

# Install internal pages
install(
    DIRECTORY src/ui/pages/
    DESTINATION share/open-browser/pages
)

# Install Lucide icon set + app icons
install(
    DIRECTORY resources/icons/
    DESTINATION share/open-browser/icons
)

# Install app icon to system icon dirs (for taskbar / launcher)
install(FILES resources/icons/hicolor/scalable/apps/io.openbrowser.Browser.svg
    DESTINATION share/icons/hicolor/scalable/apps)
install(FILES resources/icons/hicolor/256x256/apps/io.openbrowser.Browser.png
    DESTINATION share/icons/hicolor/256x256/apps)
install(FILES resources/icons/hicolor/128x128/apps/io.openbrowser.Browser.png
    DESTINATION share/icons/hicolor/128x128/apps)
install(FILES resources/icons/hicolor/64x64/apps/io.openbrowser.Browser.png
    DESTINATION share/icons/hicolor/64x64/apps)
install(FILES resources/icons/hicolor/48x48/apps/io.openbrowser.Browser.png
    DESTINATION share/icons/hicolor/48x48/apps)
install(FILES resources/icons/hicolor/32x32/apps/io.openbrowser.Browser.png
    DESTINATION share/icons/hicolor/32x32/apps)

# Install GTK theme
install(
    FILES src/themes/open-browser.css
    DESTINATION share/open-browser/themes
)

# Package metadata
set(CPACK_PACKAGE_NAME "open-browser")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_CONTACT "Open Browser Contributors <hello@openbrowser.io>")
set(CPACK_DEBIAN_PACKAGE_DEPENDS
    "libgtk-4-1, libwebkit2gtk-4.1-0, libsoup-3.0-0, libsecret-1-0"
)
include(CPack)
