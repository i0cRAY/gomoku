add_executable(format_tests
        save_format_test.cpp
)
target_link_libraries(format_tests PRIVATE format GTest::gtest GTest::gtest_main)

gtest_discover_tests(format_tests)
