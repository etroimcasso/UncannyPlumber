# Test runner. Auto-discovers any *.cpp in tests/ so that adding a test file requires no CMake change.

file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "*.cpp")

if(TEST_SOURCES)
    add_executable(uncannyplumber-tests ${TEST_SOURCES})
    # Linked exactly as the shipped binary links it (src/CMakeLists.txt). A suite linked differently
    # from the program cannot catch a linkage defect in the program.
    target_link_libraries(uncannyplumber-tests PRIVATE
        uncannyplumber-lib
        GTest::gtest_main
    )
    target_compile_features(uncannyplumber-tests PRIVATE cxx_std_20)
    include(GoogleTest)
    gtest_discover_tests(uncannyplumber-tests)
endif()
