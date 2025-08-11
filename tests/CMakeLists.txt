enable_testing()

add_executable(
  unit_tests
  unit.cpp
)

add_executable(
  stress_tests
  stress.cpp
)

target_link_libraries(unit_tests
  PRIVATE kv_storage GTest::gtest_main
)

target_link_libraries(stress_tests
  PRIVATE kv_storage GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(unit_tests)
gtest_discover_tests(stress_tests)
