cmake_minimum_required(VERSION 3.16)

project(connector_common LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PROTOBUF REQUIRED protobuf)
pkg_check_modules(GRPC REQUIRED grpc++)

find_program(PROTOC protoc REQUIRED)
find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)

set(PROTO_FILE ${CMAKE_CURRENT_SOURCE_DIR}/proto/instrument/v1/instrument.proto)
set(GENERATED_DIR ${CMAKE_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${GENERATED_DIR})

set(GENERATED_PROTO_SRCS
    ${GENERATED_DIR}/instrument.pb.cc
    ${GENERATED_DIR}/instrument.grpc.pb.cc)
set(GENERATED_PROTO_HDRS
    ${GENERATED_DIR}/instrument.pb.h
    ${GENERATED_DIR}/instrument.grpc.pb.h)

add_custom_command(
    OUTPUT ${GENERATED_PROTO_SRCS} ${GENERATED_PROTO_HDRS}
    COMMAND ${PROTOC}
    ARGS --grpc_out=${GENERATED_DIR}
         --cpp_out=${GENERATED_DIR}
         -I ${CMAKE_CURRENT_SOURCE_DIR}/proto/instrument/v1
         -I /usr/include
         --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
         ${PROTO_FILE}
    DEPENDS ${PROTO_FILE}
    COMMENT "Generating connector-common gRPC/protobuf sources"
)

add_library(connector_common STATIC
    src/Instrument.cpp
    src/InstrumentClient.cpp
    ${GENERATED_PROTO_SRCS}
)

target_include_directories(connector_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${GENERATED_DIR}
    ${PROTOBUF_INCLUDE_DIRS}
    ${GRPC_INCLUDE_DIRS}
)

target_link_libraries(connector_common PUBLIC
    ${PROTOBUF_LIBRARIES}
    ${GRPC_LIBRARIES}
)

target_compile_options(connector_common PRIVATE -Wall -Wextra)
