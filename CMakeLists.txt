cmake_minimum_required(VERSION 3.20)
project(spsc-logger LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -O2)

add_executable(basic examples/basic.cpp)
target_include_directories(basic PRIVATE include)
