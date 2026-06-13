set(BROWSER_SOURCES
    main.cpp
    browser_window.cpp
    url_resolver.cpp
    settings_manager.cpp
    bookmark_manager.cpp
    history_manager.cpp
    download_manager.cpp
    password_manager.cpp
    ad_blocker.cpp
)

# Production-ready modular UI components
set(UI_SOURCES
    ui/tab_strip.cpp
    ui/window_controls.cpp
    ui/address_bar.cpp
    ui/navigation_bar.cpp
    ui/browser_shell.cpp
)

add_executable(open-browser ${BROWSER_SOURCES} ${UI_SOURCES})

target_include_directories(open-browser PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/ui"
    "${CMAKE_CURRENT_BINARY_DIR}/.."
)

target_link_libraries(open-browser PRIVATE
    PkgConfig::GTK4
    PkgConfig::WEBKIT
    PkgConfig::LIBSOUP
    PkgConfig::GLIB
    PkgConfig::GIO
    PkgConfig::LIBSECRET
    nlohmann_json::nlohmann_json
)

# Copy internal pages to build dir for development convenience
add_custom_command(TARGET open-browser POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/ui/pages"
        "${CMAKE_CURRENT_BINARY_DIR}/pages"
    COMMENT "Copying internal pages to build directory"
)

# Copy Lucide icons to build dir for development convenience
add_custom_command(TARGET open-browser POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/resources/icons"
        "${CMAKE_CURRENT_BINARY_DIR}/../resources/icons"
    COMMENT "Copying Lucide icons to build directory"
)

# Copy theme CSS to build dir for development convenience
add_custom_command(TARGET open-browser POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/themes"
        "${CMAKE_CURRENT_BINARY_DIR}/../src/themes"
    COMMENT "Copying theme CSS to build directory"
)

# Install the binary
install(TARGETS open-browser
    RUNTIME DESTINATION bin
)
