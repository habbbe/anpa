cmake_minimum_required(VERSION 3.8)

project(parsimon)

set(CMAKE_CXX_STANDARD 17)

add_library(parsimon INTERFACE)
target_include_directories(parsimon INTERFACE include/)

install(
    DIRECTORY include/parsimon
    DESTINATION include
    )


option(BUILD_TESTS "Builds the tests" true)

if(${BUILD_TESTS})
    enable_testing()
    add_subdirectory(test)
endif(${BUILD_TESTS})
