cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})


include(FetchContent)
include(cmake/gRPC.cmake)

add_subdirectory(engine)