cmake_minimum_required(VERSION 2.15)
project(iguana)

if(MSVC)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
else()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pthread -std=c++20")
endif(MSVC)
set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -ldl")
set(IGUANA_DIR ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(
	${IGUANA_DIR}
	${IGUANA_DIR}/third_party/msgpack/include
	${IGUANA_DIR}/third_party/frozen/include
	${IGUANA_DIR}/third_party/doctest
)

set(EXAMPLE
		example/example.cpp
)

set(JSON_EXAMPLE
	example/json_example.cpp
)

set(XML_EXAMPLE
	example/xml_example.cpp
)

set(MSGPACK_EXAMPLE
	example/msgpack_example.cpp
)

set(TEST example/test.cpp)

add_executable(example 	${EXAMPLE})
add_executable(json_example 	${JSON_EXAMPLE})
add_executable(xml_example 		${XML_EXAMPLE})
add_executable(msgpack_example 	${MSGPACK_EXAMPLE})
add_executable(test 	${TEST})
