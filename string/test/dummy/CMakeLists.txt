cmake_minimum_required(VERSION 3.10)
project(dummy)

set(CMAKE_CXX_FLAGS "-g -pthread")

include_directories(../..)
include_directories(~/zxy/)


add_executable(test ../string-view-test.cc ../../string-view.cc)
target_link_libraries(test /usr/local/lib/libgtestc.a)
