add_executable(core_tests
        types_test.cpp
        board_test.cpp
        rules_test.cpp
        move_history_test.cpp
        game_state_test.cpp
        game_record_test.cpp
        replay_cursor_test.cpp
)
target_link_libraries(core_tests PRIVATE core GTest::gtest GTest::gtest_main)

gtest_discover_tests(core_tests)
