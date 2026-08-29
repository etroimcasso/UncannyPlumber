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
