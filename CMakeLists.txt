cmake_minimum_required(VERSION 3.21)

project(
    fauxboy
    VERSION 0.1.0
    LANGUAGES CXX
)

include(cmake/PreventInSourceBuild.cmake)
include(cmake/Options.cmake)

add_library(
    fauxboy_lib ${FAUXBOY_BUILD_TYPE}
    # include
    include/fauxboy/cpu.hpp
    include/fauxboy/bus.hpp
    include/fauxboy/address.hpp
    include/fauxboy/util.hpp
    include/fauxboy/register.hpp
    # src
    src/cpu.cpp
    src/bus.cpp
)

add_library(fauxboy::fauxboy ALIAS fauxboy_lib)

set_target_properties(
    fauxboy_lib PROPERTIES
    LINKER_LANGUAGE CXX
    VERSION "${PROJECT_VERSION}"
    SOVERSION "${PROJECT_VERSION_MAJOR}"
)

target_include_directories(
    fauxboy_lib
    PRIVATE "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/fauxboy>"
    INTERFACE "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

add_executable(
    fauxboy
    # include
    # src
    src/main.cpp
)

target_include_directories(fauxboy
    PRIVATE fauxboy::fauxboy
)

if (FAUXBOY_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif ()
