cmake_minimum_required(VERSION 3.5)
project(iguana)

message(STATUS "C++ Compiler Version: ${CMAKE_CXX_COMPILER_VERSION}")

if(MSVC)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
add_compile_options(/utf-8)
ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/bigobj>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus>")
else()

if(NOT LINKLIBC++)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pthread -g ")
message(STATUS "link libstdc++")
else()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pthread -g -stdlib=libc++")
message(STATUS "link libc++")
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif(MSVC)
set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE} -ldl")
set(IGUANA_DIR ${CMAKE_CURRENT_SOURCE_DIR})

if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")

add_definitions(-DTHROW_UNKNOWN_KEY)

add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/bigobj>")

option(ENABLE_SEQUENTIAL_PARSE "parse json sequential more efficient if the json fields sequences are the same with struct fields" OFF)
if (${ENABLE_SEQUENTIAL_PARSE})
    ADD_DEFINITIONS(-DSEQUENTIAL_PARSE)
endif ()

option(HAS_RAPIDJSON "import rapidjson" OFF)
if (${HAS_RAPIDJSON})
	add_definitions(-DHAS_RAPIDJSON)
	include_directories(../rapidjson/include)
	ADD_DEFINITIONS(-DRAPIDJSON_HAS_STDSTRING)
endif()

option(HAS_RAPIDYAML "import rapidyaml" OFF)
if (${HAS_RAPIDYAML})
	add_definitions(-DHAS_RAPIDYAML)
endif()

include_directories(
	${IGUANA_DIR}
)

if(CMAKE_COMPILER_IS_GNUCXX)
link_libraries(stdc++fs)
endif()

function(iguana_fast_validation_target target)
    if(MSVC)
        target_compile_options(${target} PRIVATE "$<$<CONFIG:Release>:/Od;/Ob0>")
    else()
        target_compile_options(${target} PRIVATE "$<$<CONFIG:Release>:-O0>")
    endif()
endfunction()

option(IGUANA_FAST_BENCHMARK_COMPILE
       "Compile benchmark targets with low optimization for faster validation builds"
       OFF)

option(IGUANA_JSON_DOM_ENABLE_AVX2
       "Compile json DOM benchmark/test targets with AVX2 on x86 hosts"
       OFF)

function(iguana_fast_benchmark_target target)
    if(IGUANA_FAST_BENCHMARK_COMPILE)
        iguana_fast_validation_target(${target})
    endif()
endfunction()

set(JSON_EXAMPLE
	example/json_example.cpp
)

set(XML_EXAMPLE
	example/xml_example.cpp
)
set(YAML_EXAMPLE
	example/yaml_example.cpp
)

set(TEST_SOME test/test_some.cpp)
set(TEST_UT test/unit_test.cpp)
set(TEST_JSON_FILES test/test_json_files.cpp)
set(TEST_GITHUB_EVENTS test/test_github_events.cpp)
set(TEST_XML test/test_xml.cpp)
set(JSONBENCHMARK benchmark/json_benchmark.cpp)
set(XMLBENCH  benchmark/xml_benchmark.cpp)
set(TEST_YAML test/test_yaml.cpp test/test_yaml_bech.cpp)
set(YAMLBENCH  benchmark/yaml_benchmark.cpp)
set(TEST_NOTHROW test/test_yaml_nothrow.cpp)
set(TEST_UTIL test/test_util.cpp)
set(TEST_XMLNOTHROW test/test_xml_nothrow.cpp)
set(TEST_PB test/test_pb.cpp)
set(TEST_PROTO test/test_proto3.cpp)
set(TEST_CPP20 test/test_cpp20.cpp)
set(TEST_JSON_DOM test/test_json_dom.cpp)
set(TEST_CONFORMANCE test/conformance/iguana_conformance.cpp)
set(PB_BENCHMARK benchmark/pb_benchmark.cpp)

add_executable(json_example 	${JSON_EXAMPLE})
add_executable(json_benchmark 	${JSONBENCHMARK})
add_executable(test_some	${TEST_SOME})
add_executable(test_ut 	${TEST_UT})
add_executable(test_json_files 	${TEST_JSON_FILES})
add_executable(test_github_events ${TEST_GITHUB_EVENTS})
add_executable(xml_example 		${XML_EXAMPLE})
add_executable(test_xml ${TEST_XML})
add_executable(xml_benchmark ${XMLBENCH})
add_executable(test_xml_nothrow ${TEST_XMLNOTHROW})
add_executable(yaml_example 		${YAML_EXAMPLE})
add_executable(test_yaml ${TEST_YAML})
add_executable(yaml_benchmark ${YAMLBENCH})
add_executable(test_nothrow ${TEST_NOTHROW})
add_executable(test_util ${TEST_UTIL})
add_executable(test_pb ${TEST_PB})
add_executable(test_cpp20 ${TEST_CPP20})
add_executable(test_json_dom ${TEST_JSON_DOM})
add_executable(iguana_conformance ${TEST_CONFORMANCE})
set_target_properties(json_benchmark PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
set_target_properties(test_cpp20 PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
set_target_properties(test_json_dom PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
target_compile_definitions(test_json_dom PRIVATE IGUANA_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")

if(MSVC AND NOT IGUANA_FAST_BENCHMARK_COMPILE)
    target_compile_options(json_benchmark PRIVATE "$<$<CONFIG:Release>:/Ob3;/Oi;/Ot>")
endif()

if(IGUANA_JSON_DOM_ENABLE_AVX2 AND
   (CMAKE_SYSTEM_PROCESSOR MATCHES "AMD64|x86_64|X86|i.86"))
    if(MSVC)
        target_compile_options(json_benchmark PRIVATE /arch:AVX2)
        target_compile_options(test_json_dom PRIVATE /arch:AVX2)
    else()
        target_compile_options(json_benchmark PRIVATE -mavx2)
        target_compile_options(test_json_dom PRIVATE -mavx2)
    endif()
endif()

set(IGUANA_NLOHMANN_JSON_DIR ""
    CACHE PATH "Path to nlohmann/json source tree")
if(EXISTS "${IGUANA_NLOHMANN_JSON_DIR}/single_include/nlohmann/json.hpp")
    target_compile_definitions(json_benchmark PRIVATE HAS_NLOHMANN_JSON)
    target_include_directories(json_benchmark PRIVATE "${IGUANA_NLOHMANN_JSON_DIR}/single_include")
endif()

set(IGUANA_YYJSON_DIR ""
    CACHE PATH "Path to yyjson source tree")
if(EXISTS "${IGUANA_YYJSON_DIR}/src/yyjson.h" AND EXISTS "${IGUANA_YYJSON_DIR}/src/yyjson.c")
    target_compile_definitions(json_benchmark PRIVATE HAS_YYJSON)
    target_include_directories(json_benchmark PRIVATE "${IGUANA_YYJSON_DIR}/src")
    target_sources(json_benchmark PRIVATE "${IGUANA_YYJSON_DIR}/src/yyjson.c")
endif()

set(IGUANA_SIMDJSON_DIR ""
    CACHE PATH "Path to simdjson source tree")
if(EXISTS "${IGUANA_SIMDJSON_DIR}/singleheader/simdjson.h" AND EXISTS "${IGUANA_SIMDJSON_DIR}/singleheader/simdjson.cpp")
    target_compile_definitions(json_benchmark PRIVATE HAS_SIMDJSON)
    target_include_directories(json_benchmark PRIVATE "${IGUANA_SIMDJSON_DIR}/singleheader")
    target_sources(json_benchmark PRIVATE "${IGUANA_SIMDJSON_DIR}/singleheader/simdjson.cpp")
endif()

foreach(tgt json_benchmark xml_benchmark yaml_benchmark)
    iguana_fast_benchmark_target(${tgt})
endforeach()

foreach(tgt json_example xml_example yaml_example
            test_some test_ut test_json_files test_github_events test_xml test_xml_nothrow
            test_yaml test_nothrow test_util test_pb test_cpp20 test_json_dom
            iguana_conformance)
    iguana_fast_validation_target(${tgt})
endforeach()

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 9)
        message(STATUS "GCC version ${CMAKE_CXX_COMPILER_VERSION}")
        add_executable(test_reflection test/test_reflection.cpp)
        iguana_fast_validation_target(test_reflection)
    endif()
else()
    add_executable(test_reflection test/test_reflection.cpp)
    iguana_fast_validation_target(test_reflection)
endif()


find_package(Protobuf QUIET)
if(Protobuf_FOUND)
    add_definitions(-DSTRUCT_PB_WITH_PROTO)
    find_package(Protobuf REQUIRED)
    if(Protobuf_FOUND)
        message(STATUS "Found Protobuf: ${Protobuf_VERSION}")
    else()
        message(STATUS "Protobuf not found")
    endif()

    include_directories(${Protobuf_INCLUDE_DIRS})
    include_directories(${CMAKE_CURRENT_BINARY_DIR})
    set(PROTO_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test/proto)
    file(GLOB PROTO_FILES ${PROTO_SRC_DIR}/*.proto)

    protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})
    message(STATUS "Proto source files: ${PROTO_SRCS}")
    message(STATUS "Proto header files: ${PROTO_HDRS}")
    
    add_executable(pb_benchmark ${PB_BENCHMARK} ${PROTO_SRCS})
    target_link_libraries(pb_benchmark ${Protobuf_LIBRARIES})
    iguana_fast_benchmark_target(pb_benchmark)
    add_executable(test_proto ${PROTO_SRCS} ${TEST_PROTO})
    target_link_libraries(test_proto ${Protobuf_LIBRARIES})
    iguana_fast_validation_target(test_proto)
    add_test(NAME test_proto COMMAND test_proto)
endif()

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
add_test(NAME test_github_events COMMAND test_github_events)
add_test(NAME test_xml COMMAND test_xml)
add_test(NAME test_yaml COMMAND test_yaml)
add_test(NAME test_nothrow COMMAND test_nothrow)
add_test(NAME test_util COMMAND test_util)
add_test(NAME test_xml_nothrow COMMAND test_xml_nothrow)
add_test(NAME test_pb COMMAND test_pb)
add_test(NAME test_cpp20 COMMAND test_cpp20)
add_test(NAME test_json_dom COMMAND test_json_dom)
set_tests_properties(test_json_files PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/test")
set_tests_properties(test_github_events PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/test")

set(PROTOBUF_CONFORMANCE_RUNNER "" CACHE FILEPATH "Path to protobuf conformance_test_runner")
if(PROTOBUF_CONFORMANCE_RUNNER)
    add_test(NAME iguana_conformance_runner
        COMMAND ${PROTOBUF_CONFORMANCE_RUNNER} --enforce_recommended $<TARGET_FILE:iguana_conformance>)
endif()

# C++26 static reflection tests (requires GCC 16+ with -freflection)
option(ENABLE_CXX26_REFLECTION "Build tests with C++26 static reflection (requires GCC 16+)" OFF)
if (ENABLE_CXX26_REFLECTION)
    message(STATUS "Building with C++26 static reflection")
    execute_process(
        COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=libstdc++.so
        OUTPUT_VARIABLE CXX26_LIBSTDCXX
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    if(CXX26_LIBSTDCXX AND EXISTS "${CXX26_LIBSTDCXX}")
        get_filename_component(CXX26_LIBSTDCXX_DIR "${CXX26_LIBSTDCXX}" DIRECTORY)
    endif()
    foreach(tgt test_some test_ut test_json_files test_github_events test_xml
                test_yaml test_nothrow test_util test_xml_nothrow test_pb
                test_reflection)
        if(TARGET ${tgt})
            set(tgt26 ${tgt}_cpp26)
            get_target_property(tgt_sources ${tgt} SOURCES)
            add_executable(${tgt26} ${tgt_sources})
            target_compile_options(${tgt26} PRIVATE -std=gnu++26 -freflection)
            iguana_fast_validation_target(${tgt26})
            if(CXX26_LIBSTDCXX_DIR)
                set_property(TARGET ${tgt26} APPEND_STRING PROPERTY LINK_FLAGS
                    " -Wl,-rpath,${CXX26_LIBSTDCXX_DIR}")
            endif()
            add_test(NAME ${tgt26} COMMAND ${tgt26})
            if(tgt STREQUAL test_json_files OR tgt STREQUAL test_github_events)
                set_tests_properties(${tgt26} PROPERTIES
                    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/test")
            endif()
        endif()
    endforeach()
endif()
