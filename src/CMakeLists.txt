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
add_link_options(-pg -fno-omit-frame-pointer)

include_directories(${CMAKE_SOURCE_DIR}/)

# We need the proper way to do this.
#
# The proper way to find python from the pybind documentation is this:
find_package(Python 3.6 COMPONENTS Interpreter Development REQUIRED)
#
# However, you cannot use the native windows python version with the msys2
# toolchain because it doesn't work (not sure why, it produces a pybind module
# example.cp311-win_amd64.pyd, but it does not seem possible to import it, but
# it does not seem possible to import it. Even though it says module not found,
# I suspect it is some kind of ABI incompatibility issue between using msys2 for
# compiling, and using the native windows python installation, because no amount 
# of modifying any of the various paths fixed it. If I could find a way to emit more
# verbose logs, it might be possible to fix).
#set(PYTHON_EXECUTABLE "/mingw64/bin/python3.10.exe")
#set(PYTHON_LIBRARY "/mingw64/lib/libpython3.10.dll.a")
#set(PYTHON_EXECUTABLE "/C/Users/john.scott/AppData/Local/Programs/Python/Python311/python.exe")
#set(PYTHON_LIBRARY "/C/Users/john.scott/AppData/Local/Programs/Python/Python311/python311.dll")

# To find the native-windows-installed python, 
# 
add_subdirectory(pybind11)
pybind11_add_module(pbtest main_python.cpp)

add_executable(spells programs/spells.cpp yaml.cpp category.cpp clinical_code.cpp
  random.cpp string_lookup.cpp config.cpp cmdline/cmdline.cpp 
  sql_debug.cpp sql_types.cpp)
target_link_libraries(spells ${ODBC_LIB_NAME} yaml-cpp)

#add_executable(main programs/main.cpp)
#target_link_libraries(main rdb odbc yaml-cpp)

add_executable(main programs/test.cpp)
target_link_libraries(main)


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

  add_executable(run-gtest gtest/string_lookup.cpp gtest/clinical_code.cpp 
    gtest/episode.cpp gtest/parser.cpp gtest/timestamp.cpp yaml.cpp 
    category.cpp clinical_code.cpp random.cpp string_lookup.cpp config.cpp
    cmdline/cmdline.cpp sql_debug.cpp sql_types.cpp)
  target_link_libraries(run-gtest gtest_main yaml-cpp ${ODBC_LIB_NAME})

  include(GoogleTest)
  gtest_discover_tests(run-gtest)

endif()