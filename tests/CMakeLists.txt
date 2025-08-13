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

include(GoogleTest)
gtest_discover_tests(unit_tests)
