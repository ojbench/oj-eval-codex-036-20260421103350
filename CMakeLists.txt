cmake_minimum_required(VERSION 3.10)
project(problem_036)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(code src/main.cpp)

# Optimize flags
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(code PRIVATE -O2 -pipe -static -s)
endif()
