cmake_minimum_required(VERSION 3.12.4) # To get std=c++20

project(rdb VERSION 0.1.0 LANGUAGES CXX)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_compile_options(-fmax-errors=5)

add_executable(main main.cpp)
target_link_libraries(main odbc)
