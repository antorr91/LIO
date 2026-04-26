cmake_minimum_required(VERSION 3.21)

project(LIO VERSION 0.2.0 LANGUAGES CXX RC)

# ── Language standard ────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD          20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Dependencies ─────────────────────────────────────────────────────────────
find_package(unofficial-webview2 CONFIG REQUIRED)

# ── Resource file (icon) ─────────────────────────────────────────────────────
set(LIO_ICON_PATH_RC "${CMAKE_CURRENT_SOURCE_DIR}/assets/LIO.ico")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/lio_resource.rc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/lio_resource.rc"
    @ONLY
)

# ── Executable ───────────────────────────────────────────────────────────────
add_executable(lio WIN32
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    "${CMAKE_CURRENT_BINARY_DIR}/lio_resource.rc"
)

target_include_directories(lio PRIVATE src)

target_link_libraries(lio PRIVATE
    unofficial::webview2::webview2
    User32
    Ole32
    Shell32
)

target_compile_definitions(lio PRIVATE UNICODE _UNICODE)

if(MSVC)
    target_compile_options(lio PRIVATE /W4 /permissive-)
endif()

# ── Copy assets next to the executable after build ───────────────────────────
set(ASSET_DIR "$<TARGET_FILE_DIR:lio>/assets")

add_custom_command(TARGET lio POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${ASSET_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/LIO.html"  "${ASSET_DIR}/LIO.html"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/LIO.ico"   "${ASSET_DIR}/LIO.ico"
    COMMENT "Copying assets to output directory"
)

# ── Install ──────────────────────────────────────────────────────────────────
install(TARGETS   lio     RUNTIME  DESTINATION .)
install(DIRECTORY assets  DESTINATION .)
