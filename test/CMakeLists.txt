project(parsimon_tests)

include_directories(
    json
    calc)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(SOURCES
    tests_main.cpp
    tests_parsers.cpp
    tests_combinators.cpp
    tests_perf.cpp
    tests_json.cpp
    tests_calc.cpp
    )

set(TEST_TARGET parsimon_tests)
add_executable(${TEST_TARGET} ${SOURCES})
target_link_libraries(${TEST_TARGET} PRIVATE parsimon)

add_test(
    NAME ${TEST_TARGET} 
    COMMAND ${TEST_TARGET}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/data)
