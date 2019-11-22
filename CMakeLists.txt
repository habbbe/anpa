cmake_minimum_required(VERSION 3.8)

project(parsimon VERSION 0.1.0)

add_library(parsimon INTERFACE)

target_compile_features(parsimon INTERFACE cxx_std_17)

target_include_directories(parsimon INTERFACE include/)


install(
    DIRECTORY include/parsimon
    DESTINATION include)


option(BUILD_TESTS "Builds the tests" false)

if(${BUILD_TESTS})
    enable_testing()
    add_subdirectory(test)
endif(${BUILD_TESTS})
