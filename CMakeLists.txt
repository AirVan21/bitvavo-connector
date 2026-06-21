cmake_minimum_required(VERSION 3.16)

project(BitvavoConnector LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(EXISTS "${CMAKE_BINARY_DIR}/conan_paths.cmake")
    include("${CMAKE_BINARY_DIR}/conan_paths.cmake")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

find_package(Boost REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(RapidJSON REQUIRED)

add_subdirectory(Dependencies/connector-network)
add_subdirectory(Dependencies/connector-common)

add_library(bitvavo_client STATIC BitvavoClient.cpp)
target_include_directories(bitvavo_client PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${RapidJSON_INCLUDE_DIRS}
)
target_link_libraries(bitvavo_client PUBLIC
    connector_network
    connector_common
    ${Boost_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
)

add_executable(BitvavoConnector main.cpp)
target_link_libraries(BitvavoConnector PRIVATE bitvavo_client)
