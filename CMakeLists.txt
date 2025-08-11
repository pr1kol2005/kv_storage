cmake_minimum_required(VERSION 3.20)
project(kv_storage)

include(FetchContent)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# cmake -B build -D BUILD_TESTS=ON
option(BUILD_TESTS "Build tests" OFF)

set(SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)
set(TESTS_DIR ${PROJECT_SOURCE_DIR}/tests)

# Abseil
FetchContent_Declare(
  abseil
  URL https://github.com/abseil/abseil-cpp/archive/refs/tags/20250512.1.zip
)
FetchContent_MakeAvailable(abseil)

add_subdirectory(${SOURCE_DIR})

if(BUILD_TESTS)
  # GoogleTest
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
  )
  FetchContent_MakeAvailable(googletest)

  add_subdirectory(${TESTS_DIR})
endif()
