# 測試共用的輔助工具（假時鐘等），只給測試使用
add_library(test_support INTERFACE)
target_include_directories(test_support INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_support INTERFACE core)

add_executable(test_support_tests
        fake_time_source_test.cpp
)
target_link_libraries(test_support_tests PRIVATE test_support GTest::gtest GTest::gtest_main)

gtest_discover_tests(test_support_tests)
