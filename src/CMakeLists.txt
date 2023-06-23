cmake_minimum_required(VERSION 3.12.4) # To get std=c++20

project(rdb VERSION 0.1.0 LANGUAGES CXX)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT ODBC_LIB_NAME)
    # The windows name
    set(ODBC_LIB_NAME odbc32)
endif()

add_compile_options(-Wall -Wextra -fmax-errors=5 -g -pg -fno-omit-frame-pointer)
add_link_options(-pg -no-pie -fno-omit-frame-pointer)

include_directories(${CMAKE_SOURCE_DIR}/)


# To find the native-windows-installed python, use the find_package function.
# Make sure that it detects the windows one and not any msys2 installations.
# Python version specified as 3.9 for compatibility with tensorflow-transform.
# 
find_package(Python3 3.9 EXACT COMPONENTS Interpreter Development REQUIRED)
add_subdirectory(pybind11)
pybind11_add_module(pbtest main_python.cpp yaml.cpp category.cpp clinical_code.cpp 
  random.cpp string_lookup.cpp config.cpp cmdline/cmdline.cpp sql_debug.cpp sql_types.cpp)
target_link_libraries(pbtest PRIVATE ${ODBC_LIB_NAME} yaml-cpp)

add_executable(spells programs/spells.cpp yaml.cpp category.cpp clinical_code.cpp
  random.cpp string_lookup.cpp config.cpp cmdline/cmdline.cpp 
  sql_debug.cpp sql_types.cpp)
target_link_libraries(spells ${ODBC_LIB_NAME} yaml-cpp)

add_executable(main programs/test.cpp)
target_link_libraries(main ${ODBC_LIB_NAME})

# Testing
set(INSTALL_GTEST OFF)
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
  )
  FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(run-gtest gtest/string_lookup.cpp gtest/clinical_code.cpp 
  gtest/episode.cpp gtest/parser.cpp gtest/timestamp.cpp yaml.cpp gtest/top_level_category.cpp
  category.cpp clinical_code.cpp random.cpp string_lookup.cpp config.cpp
  cmdline/cmdline.cpp sql_debug.cpp sql_types.cpp)
target_link_libraries(run-gtest gtest_main yaml-cpp ${ODBC_LIB_NAME})

include(GoogleTest)
gtest_discover_tests(run-gtest)