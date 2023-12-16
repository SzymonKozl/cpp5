cmake_minimum_required(VERSION 3.25.1)
project(cpp5 CXX)

set(CMAKE_CXX_STANDARD 20)

include_directories(.)

add_executable(stack
        stack.h stack_example.cc)
