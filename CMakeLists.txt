cmake_minimum_required(VERSION 3.8)

project(parsimon)

set(CMAKE_CXX_STANDARD 17)

include_directories(src)
include_directories(src/parsimon)

set(SOURCES
    src/test/tests_main.cpp
    src/test/tests_parsers.cpp
    src/test/tests_combinators.cpp
    src/test/tests_perf.cpp
    src/test/tests_json.cpp
) 

add_executable(parsimon_test ${SOURCES} )
