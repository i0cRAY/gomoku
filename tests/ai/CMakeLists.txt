add_executable(ai_tests
        evaluator_test.cpp
        candidates_test.cpp
)
target_link_libraries(ai_tests PRIVATE ai test_support GTest::gtest GTest::gtest_main)

gtest_discover_tests(ai_tests)
