enable_testing()

add_executable(
  unit_tests
  unit.cpp
)

target_link_libraries(unit_tests
  PRIVATE GTest::gtest_main
)

target_include_directories(unit_tests
  PRIVATE ${INCLUDE_DIR}
)

add_executable(
  time_tests
  time.cpp
)

target_link_libraries(time_tests
  PRIVATE GTest::gtest_main
)

target_include_directories(time_tests
  PRIVATE ${INCLUDE_DIR}
)

add_executable(
  stress_tests
  stress.cpp
)

target_link_libraries(stress_tests
  PRIVATE GTest::gtest_main
)

target_include_directories(stress_tests
  PRIVATE ${INCLUDE_DIR}
)

include(GoogleTest)
gtest_discover_tests(unit_tests)
gtest_discover_tests(time_tests)
gtest_discover_tests(stress_tests)
