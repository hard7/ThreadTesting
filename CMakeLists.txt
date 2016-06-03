cmake_minimum_required(VERSION 3.4)
project(ThreadTesting)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp)
add_executable(ThreadTesting ${SOURCE_FILES})
target_link_libraries(ThreadTesting pthread)