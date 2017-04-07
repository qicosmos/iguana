cmake_minimum_required(VERSION 2.8)
project(iguana)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -std=c++14")
set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -ldl")
set(IGUANA_DIR ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(
	${IGUANA_DIR}
	${IGUANA_DIR}/third_party/msgpack/include
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
		
add_executable(json_example 	${JSON_EXAMPLE})
add_executable(xml_example 		${XML_EXAMPLE})
add_executable(msgpack_example 	${MSGPACK_EXAMPLE})
