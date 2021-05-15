cmake_minimum_required(VERSION 3.8)

project(anpa VERSION 0.1.0)

add_library(anpa INTERFACE)

target_compile_features(anpa INTERFACE cxx_std_17)

target_include_directories(anpa INTERFACE include/)


install(
    DIRECTORY include/anpa
    DESTINATION include)


option(BUILD_TESTS "Builds the tests" false)

if(${BUILD_TESTS})
    enable_testing()
    add_subdirectory(test)
endif(${BUILD_TESTS})
