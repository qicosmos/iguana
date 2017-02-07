cmake_minimum_required(VERSION 2.8)
project(iguana)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -std=c++14")
set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -ldl")

set(SOURCE_FILES
        example.cpp
        )
add_executable(iguana ${SOURCE_FILES})