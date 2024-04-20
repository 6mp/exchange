# This file handles building each task as sub-project.
#
# Each task becomes an independent project in which you can experiment.
#
# Tasks are added as separate sub-projects so as compilation errors in
# one task don't affect another task.

project(engine)

set(CMAKE_CXX_STANDARD 20)
set(FMT_TEST FF CACHE INTERNAL "disabling fmt tests")

FetchContent_Declare(
        fmt
        GIT_REPOSITORY  https://github.com/fmtlib/fmt.git
        GIT_TAG         10.2.1
        GIT_PROGRESS    TRUE
        USES_TERMINAL_DOWNLOAD TRUE
)
FetchContent_MakeAvailable(fmt)

file(GLOB_RECURSE SOURCES
        src/*.cpp
)
list(REMOVE_ITEM SOURCES "src/main.cpp")

add_executable(engine ${SOURCES} "src/main.cpp")
target_include_directories(engine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(engine grpc++ libprotobuf fmt::fmt)
target_add_protobuf(engine ${ROOT_DIR}/protos/Orderbook.proto)
