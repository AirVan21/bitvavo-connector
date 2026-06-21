cmake_minimum_required(VERSION 3.16)

project(connector_common LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Protobuf ──────────────────────────────────────────────────────────────────
# Try cmake config (installed from source), then pkg-config (apt), then raw find.
find_package(Protobuf QUIET)
if(NOT Protobuf_FOUND)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(PROTOBUF QUIET protobuf)
    endif()
endif()
if(NOT Protobuf_FOUND AND NOT PROTOBUF_FOUND)
    find_path(PROTOBUF_INCLUDE_DIRS google/protobuf/message.h REQUIRED)
    find_library(PROTOBUF_LIBRARIES protobuf REQUIRED)
    set(PROTOBUF_FOUND TRUE)
    message(STATUS "Found protobuf via find_library: ${PROTOBUF_LIBRARIES}")
endif()

# ── gRPC ──────────────────────────────────────────────────────────────────────
find_package(gRPC QUIET)
if(NOT gRPC_FOUND)
    if(NOT PkgConfig_FOUND)
        find_package(PkgConfig QUIET)
    endif()
    if(PkgConfig_FOUND)
        pkg_check_modules(GRPC QUIET grpc++)
    endif()
endif()
if(NOT gRPC_FOUND AND NOT GRPC_FOUND)
    find_path(GRPC_INCLUDE_DIRS grpcpp/grpcpp.h REQUIRED)
    find_library(GRPC_LIBRARIES grpc++ REQUIRED)
    set(GRPC_FOUND TRUE)
    message(STATUS "Found grpc++ via find_library: ${GRPC_LIBRARIES}")
endif()

# ── Tools ─────────────────────────────────────────────────────────────────────
find_program(PROTOC protoc REQUIRED)
find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)

# ── Proto codegen ─────────────────────────────────────────────────────────────
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

# ── Library ───────────────────────────────────────────────────────────────────
add_library(connector_common STATIC
    src/Instrument.cpp
    src/InstrumentClient.cpp
    ${GENERATED_PROTO_SRCS}
)

# Resolve include dirs and link libs regardless of which detection path was used.
set(_PROTO_INCS  $<$<BOOL:${Protobuf_INCLUDE_DIRS}>:${Protobuf_INCLUDE_DIRS}>
                 $<$<BOOL:${PROTOBUF_INCLUDE_DIRS}>:${PROTOBUF_INCLUDE_DIRS}>)
set(_GRPC_INCS   $<$<BOOL:${gRPC_INCLUDE_DIRS}>:${gRPC_INCLUDE_DIRS}>
                 $<$<BOOL:${GRPC_INCLUDE_DIRS}>:${GRPC_INCLUDE_DIRS}>)
set(_PROTO_LIBS  $<$<BOOL:${Protobuf_LIBRARIES}>:${Protobuf_LIBRARIES}>
                 $<$<BOOL:${PROTOBUF_LIBRARIES}>:${PROTOBUF_LIBRARIES}>)
set(_GRPC_LIBS   $<$<BOOL:${gRPC_LIBRARIES}>:${gRPC_LIBRARIES}>
                 $<$<BOOL:${GRPC_LIBRARIES}>:${GRPC_LIBRARIES}>)

target_include_directories(connector_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${GENERATED_DIR}
    ${_PROTO_INCS}
    ${_GRPC_INCS}
)

target_link_libraries(connector_common PUBLIC
    ${_PROTO_LIBS}
    ${_GRPC_LIBS}
)

target_compile_options(connector_common PRIVATE -Wall -Wextra)
