cmake_minimum_required(VERSION 2.15)
project(iguana)

if(MSVC)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++20")
ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
else()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pthread -g -std=c++20")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif(MSVC)
set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -ldl")
set(IGUANA_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_definitions(-DTHROW_UNKNOWN_KEY)

option(HAS_RAPIDJSON "import rapidjson" OFF)
if (${HAS_RAPIDJSON})
	add_definitions(-DHAS_RAPIDJSON)
	include_directories(../rapidjson/include)
	ADD_DEFINITIONS(-DRAPIDJSON_HAS_STDSTRING)
endif()

include_directories(
	${IGUANA_DIR}
	${IGUANA_DIR}/third_party/frozen/include
	${IGUANA_DIR}/third_party/rapidxml
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
set(YAML_EXAMPLE
	example/yaml_example.cpp
)

set(TEST_SOME test/test.cpp)
set(TEST_UT test/unit_test.cpp)
set(TEST_JSON_FILES test/test_json_files.cpp)
set(TEST_XML test/test_xml.cpp)
set(BENCHMARK benchmark/benchmark.cpp)
set(XMLBENCH  benchmark/xml_benchmark.cpp)
set(TEST_YAML test/test_yaml.cpp)

add_executable(example 	${EXAMPLE})
add_executable(json_example 	${JSON_EXAMPLE})
add_executable(xml_example 		${XML_EXAMPLE})
add_executable(yaml_example 		${YAML_EXAMPLE})
add_executable(test_some	${TEST_SOME})
add_executable(test_ut 	${TEST_UT})
add_executable(test_json_files 	${TEST_JSON_FILES})
add_executable(test_xml ${TEST_XML})
add_executable(benchmark 	${BENCHMARK})
add_executable(xml_benchmark ${XMLBENCH})
add_executable(test_yaml ${TEST_YAML})


# unit test
option(BUILD_UNIT_TESTS "Build unit tests" ON)
if (BUILD_UNIT_TESTS)
    enable_testing()
endif ()

# coverage test
option(COVERAGE_TEST "Build with unit test coverage" OFF)
if(COVERAGE_TEST)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage --coverage")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-instr-generate -fcoverage-mapping")
    endif()
endif()

add_test(NAME test_some COMMAND test_some)
add_test(NAME test_ut COMMAND test_ut)
add_test(NAME test_json_files COMMAND test_json_files)
add_test(NAME test_xml COMMAND test_xml)
add_test(NAME test_yaml COMMAND test_yaml)
