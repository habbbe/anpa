cmake_minimum_required(VERSION 3.8)

project(parsimon)

add_library(parsimon INTERFACE)

target_compile_features(parsimon INTERFACE cxx_std_17)

target_include_directories(parsimon INTERFACE include/)


install(
    DIRECTORY include/parsimon
    DESTINATION include)


option(RUN_TESTS "Builds the tests" true)

if(${RUN_TESTS})
    enable_testing()
    add_subdirectory(test)
endif(${RUN_TESTS})
