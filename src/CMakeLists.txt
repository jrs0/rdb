cmake_minimum_required(VERSION 3.12.4) # To get std=c++20

project(rdb VERSION 0.1.0 LANGUAGES CXX)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_compile_options(-fmax-errors=5 -g -pg -fno-omit-frame-pointer)
add_link_options(-pg -fno-omit-frame-pointer)

add_library(rdb SHARED yaml.cpp category.cpp clinical_code.cpp)

add_executable(main main.cpp )
target_link_libraries(main rdb odbc yaml-cpp)

# Compile tests if they are enabled
if(WITH_TESTS)
  set(INSTALL_GTEST OFF)
  include(FetchContent)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
    )
    FetchContent_MakeAvailable(googletest)

  enable_testing()

  add_executable(run-gtest gtest/string_lookup.cpp gtest/clinical_code.cpp gtest/episode.cpp)
  target_link_libraries(run-gtest gtest_main rdb yaml-cpp odbc)

  include(GoogleTest)
  gtest_discover_tests(run-gtest)

endif()
