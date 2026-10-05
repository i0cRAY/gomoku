add_executable(ui_tests
        board_geometry_test.cpp
)
target_link_libraries(ui_tests PRIVATE ui GTest::gtest GTest::gtest_main)

gtest_discover_tests(ui_tests)
