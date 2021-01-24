cmake_minimum_required(VERSION 3.13)

project(protoflat LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(${PROJECT_NAME} INTERFACE)
target_sources(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include/protoflat.h)
target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

option(${PROJECT_NAME}_BUILD_TESTS "Build tests" ON)

set(protobuf_BUILD_LIBPROTOC ON CACHE BOOL "")
set(protobuf_BUILD_TESTS OFF CACHE BOOL "")
set(protobuf_WITH_ZLIB OFF CACHE BOOL "")
set(protobuf_MSVC_STATIC_RUNTIME OFF CACHE BOOL "")
add_subdirectory(submodules/protobuf/cmake)

add_executable(protoc-gen-protoflat
    ${CMAKE_CURRENT_SOURCE_DIR}/protoc-gen-protoflat/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/protoc-gen-protoflat/protoflat_generator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/protoc-gen-protoflat/protoflat_generator.cpp)
target_link_libraries(protoc-gen-protoflat libprotobuf libprotoc)

if(${${PROJECT_NAME}_BUILD_TESTS})
    set(CATCH_BUILD_TESTING OFF CACHE BOOL "")
    set(CATCH_INSTALL_DOCS OFF CACHE BOOL "")
    set(CATCH_INSTALL_HELPERS OFF CACHE BOOL "")
    add_subdirectory(submodules/catch2)

    add_executable(${PROJECT_NAME}-tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/tests.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test.proto
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test.pb.h
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/test.pb.cc)
    target_link_libraries(${PROJECT_NAME}-tests ${PROJECT_NAME})
    target_link_libraries(${PROJECT_NAME}-tests libprotobuf)
    target_link_libraries(${PROJECT_NAME}-tests Catch2)
endif()
