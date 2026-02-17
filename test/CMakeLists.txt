cmake_minimum_required(VERSION 3.21)

project(
    fauxboy_test
    LANGUAGES CXX
)

if (PROJECT_IS_TOP_LEVEL)
    find_package(fauxboy REQUIRED)
    enable_testing()
endif ()

find_package(Catch2 3 REQUIRED)

add_executable(
    unit_tests
    # include
    # src
    src/tests.cpp
)

set_target_properties(
    unit_tests PROPERTIES
    LINKER_LANGUAGE CXX
)

target_include_directories(
    unit_tests
    PRIVATE "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

target_include_directories(
    unit_tests SYSTEM
    PRIVATE "$<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>"
)

target_link_libraries(
    unit_tests
    PRIVATE fauxboy::fauxboy
    PRIVATE Catch2::Catch2WithMain
)

include(CTest)
include(Catch)
catch_discover_tests(unit_tests)
