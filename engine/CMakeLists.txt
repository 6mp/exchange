# This file handles building each task as sub-project.
#
# Each task becomes an independent project in which you can experiment.
#
# Tasks are added as separate sub-projects so as compilation errors in
# one task don't affect another task.

project(engine)

set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE SOURCES
        src/*.cpp
)
list(REMOVE_ITEM SOURCES "src/main.cpp")

add_executable(engine ${SOURCES} "src/main.cpp")
target_include_directories(engine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(engine grpc++ libprotobuf)
target_add_protobuf(engine ${ROOT_DIR}/protos/Orderbook.proto)
