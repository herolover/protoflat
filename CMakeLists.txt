cmake_minimum_required(VERSION 3.16)

project(protoflat LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(${PROJECT_NAME} INTERFACE)
target_sources(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include/protoflat.h)
target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

option(${PROJECT_NAME}_BUILD_TESTS "Build tests" ON)
option(${PROJECT_NAME}_BUILD_BENCHMARK "Build benchmark" ON)

set(protobuf_BUILD_LIBPROTOC ON CACHE BOOL "")
set(protobuf_BUILD_PROTOC_BINARIES ON CACHE BOOL "")
set(protobuf_BUILD_TESTS OFF CACHE BOOL "")
set(protobuf_WITH_ZLIB OFF CACHE BOOL "")
set(protobuf_MSVC_STATIC_RUNTIME OFF CACHE BOOL "")
add_subdirectory(submodules/protobuf/cmake)

add_executable(protoc-gen-protoflat
    ${CMAKE_CURRENT_SOURCE_DIR}/protoc-gen-protoflat/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/protoc-gen-protoflat/protoflat_generator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/protoc-gen-protoflat/protoflat_generator.cpp)
target_link_libraries(protoc-gen-protoflat libprotobuf libprotoc protoflat)

if(${${PROJECT_NAME}_BUILD_TESTS} OR ${${PROJECT_NAME}_BUILD_BENCHMARK})
    file(GLOB PROTO_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/tests "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.proto")
    foreach(PROTO_FILE ${PROTO_FILES})
        string(REGEX REPLACE "(.*)\.proto" "\\1" PROTO_NAME ${PROTO_FILE})
        set(PROTO_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/tests/${PROTO_NAME}.pb.h")
        set(PROTO_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/tests/${PROTO_NAME}.pb.cc")
        set(PROTOFLAT_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/tests/${PROTO_NAME}.protoflat.h")
        set(PROTOFLAT_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/tests/${PROTO_NAME}.protoflat.cpp")
        add_custom_command(
            OUTPUT ${PROTO_HEADER} ${PROTO_SOURCE} ${PROTOFLAT_HEADER} ${PROTOFLAT_SOURCE}
            COMMAND $<TARGET_FILE:protoc> --cpp_out=. --plugin=protoc-gen-protoflat=$<TARGET_FILE:protoc-gen-protoflat> --protoflat_out=. ${PROTO_FILE}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests/
            DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/${PROTO_FILE} protoc-gen-protoflat VERBATIM
        )
        list(APPEND PROTO_SOURCES ${PROTO_HEADER} ${PROTO_SOURCE} ${PROTOFLAT_HEADER} ${PROTOFLAT_SOURCE})
    endforeach()
endif()

if(${${PROJECT_NAME}_BUILD_TESTS})
    set(CATCH_BUILD_TESTING OFF CACHE BOOL "")
    set(CATCH_INSTALL_DOCS OFF CACHE BOOL "")
    set(CATCH_INSTALL_HELPERS OFF CACHE BOOL "")
    add_subdirectory(submodules/catch2)

    add_executable(${PROJECT_NAME}-tests
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/tests.cpp
        ${PROTO_SOURCES})
    target_link_libraries(${PROJECT_NAME}-tests ${PROJECT_NAME} libprotobuf Catch2)
endif()

if(${${PROJECT_NAME}_BUILD_BENCHMARK})
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "")
    add_subdirectory(submodules/benchmark)

    add_executable(${PROJECT_NAME}-benchmark
        ${CMAKE_CURRENT_SOURCE_DIR}/benchmark/main.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/benchmark/protoflat.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/benchmark/proto3.cpp
        ${PROTO_SOURCES})
    target_link_libraries(${PROJECT_NAME}-benchmark ${PROJECT_NAME} benchmark::benchmark libprotobuf)
endif()
