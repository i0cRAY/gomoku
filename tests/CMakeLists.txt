include(GoogleTest)

add_executable(unit_tests smoke_test.cpp)
target_link_libraries(unit_tests PRIVATE GTest::gtest GTest::gtest_main)

gtest_discover_tests(unit_tests)
