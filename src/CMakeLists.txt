# Port library — main.cpp excluded so tests can link the lib without pulling in the program entry
# point. Retro++ (retropp::engine) is the platform boundary; it is linked PUBLIC so the binary and
# tests see the retropp:: surface transitively.
add_library(uncannyplumber-lib STATIC
    version.cpp
)
target_include_directories(uncannyplumber-lib PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
)
target_compile_definitions(uncannyplumber-lib PUBLIC UNCANNYPLUMBER_VERSION="${PROJECT_VERSION}")
target_link_libraries(uncannyplumber-lib PUBLIC
    retropp::engine
    spdlog::spdlog_header_only
)
target_compile_features(uncannyplumber-lib PUBLIC cxx_std_20)

# Port binary. The CMake target stays lowercase because that is what the build refers to
# everywhere; the artifact a player sees is a proper noun.
add_executable(uncannyplumber main.cpp)
target_link_libraries(uncannyplumber PRIVATE uncannyplumber-lib)
target_compile_features(uncannyplumber PRIVATE cxx_std_20)
set_target_properties(uncannyplumber PROPERTIES OUTPUT_NAME "UncannyPlumber")

# ── A game, not a command-line program ────────────────────────────────────────
# Both desktop platforms decide what an executable IS from how it is built, and the default on each
# is "a terminal program". Double-clicked, a bare Mach-O is handed to Terminal.app, and a
# console-subsystem Windows binary is given a console window whether it wants one or not. Neither is
# anything to do with the game's own output — they happen before main runs.
if(APPLE)
    # An .app is what the Finder launches directly, what a signature and a notarization ticket
    # attach to, and what carries the identifier and version. The executable inside it keeps the
    # artifact name, so the bundle is UncannyPlumber.app/Contents/MacOS/UncannyPlumber.
    set_target_properties(uncannyplumber PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/cmake/UncannyPlumberBundleInfo.plist.in"
        MACOSX_BUNDLE_BUNDLE_NAME "UncannyPlumber"
        MACOSX_BUNDLE_EXECUTABLE_NAME "UncannyPlumber"
        # Chosen once and never changed: the player's saves and extracted assets are found under
        # this identity, and a notarization ticket is bound to it. Changing it strands both.
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.uncannyplumber.uncannyplumber"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_COPYRIGHT "GNU Affero General Public License v3.0")
elseif(WIN32)
    # WIN32_EXECUTABLE links against the windows subsystem, which is what stops the console window
    # from appearing. It normally also demands WinMain — but the engine defines SDL_MAIN_HANDLED and
    # calls SDL_SetMainReady() itself, so the game keeps a plain main() and the entry point is named
    # explicitly rather than pulling in SDL's main shim to bridge the two.
    set_target_properties(uncannyplumber PROPERTIES WIN32_EXECUTABLE TRUE)
    if(MSVC)
        target_link_options(uncannyplumber PRIVATE /ENTRY:mainCRTStartup)
    endif()
endif()

# ── The shipped binary carries only what it runs ──────────────────────────────
# Release is already the default build type; the linker drops what nothing reaches and the symbol
# table comes off. A Debug build keeps all of it. MSVC's Release link already does both.
if(NOT MSVC)
    if(APPLE)
        target_link_options(uncannyplumber PRIVATE $<$<NOT:$<CONFIG:Debug>>:-Wl,-dead_strip>)
    else()
        target_compile_options(uncannyplumber-lib PRIVATE
            $<$<NOT:$<CONFIG:Debug>>:-ffunction-sections>
            $<$<NOT:$<CONFIG:Debug>>:-fdata-sections>)
        target_compile_options(uncannyplumber PRIVATE
            $<$<NOT:$<CONFIG:Debug>>:-ffunction-sections>
            $<$<NOT:$<CONFIG:Debug>>:-fdata-sections>)
        target_link_options(uncannyplumber PRIVATE $<$<NOT:$<CONFIG:Debug>>:-Wl,--gc-sections>)
    endif()

    # Apple's -x keeps global symbols (the engine's baked-routine anchors, once there are any) and
    # drops the local ones; elsewhere the executable's whole symbol table goes.
    if(CMAKE_STRIP AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        if(APPLE)
            set(uncannyplumber_strip_options -x)
        else()
            set(uncannyplumber_strip_options "")
        endif()
        add_custom_command(TARGET uncannyplumber POST_BUILD
            COMMAND "${CMAKE_STRIP}" ${uncannyplumber_strip_options} "$<TARGET_FILE:uncannyplumber>"
            COMMENT "Stripping symbols from the shipped binary")
    endif()
endif()

# ── Sign the bundle, last, after everything that modifies it ──────────────────
# Apple silicon will not run an unsigned binary at all, so the linker ad-hoc signs every build. The
# strip above edits the binary afterwards, which invalidates that signature — and a bundle's
# signature has to cover its Info.plist and seal its resources besides, which a linker signature over
# the executable alone never does. Left that way macOS calls the result damaged rather than
# unsigned, and refuses to open it at all: a broken signature is not something a player can wave
# past, where an absent one is.
#
# Ad-hoc means a signature with no identity behind it: enough to run, not enough for someone else's
# machine to trust a download. Signing for distribution replaces this and belongs to the release.
if(APPLE)
    add_custom_command(TARGET uncannyplumber POST_BUILD
        COMMAND codesign --force --sign - "$<TARGET_BUNDLE_DIR:uncannyplumber>"
        COMMAND codesign --verify --strict "$<TARGET_BUNDLE_DIR:uncannyplumber>"
        COMMENT "Signing the app bundle")
endif()
