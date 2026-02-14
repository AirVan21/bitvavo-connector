cmake_minimum_required(VERSION 3.16)

# Set the project name and specify C++20 as the standard
project(BitvavoConnector LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include Conan-generated files (if available)
if(EXISTS "${CMAKE_BINARY_DIR}/conan_paths.cmake")
    include("${CMAKE_BINARY_DIR}/conan_paths.cmake")
endif()

# Add cmake module path for custom find modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Find Boost library
find_package(Boost REQUIRED)

# Find OpenSSL library
find_package(OpenSSL REQUIRED)

# Find RapidJSON library
find_package(RapidJSON REQUIRED)

# Add connector-network submodule
add_subdirectory(Dependencies/connector-network)

# Library target (for use as submodule dependency)
add_library(bitvavo_client STATIC BitvavoClient.cpp)
target_include_directories(bitvavo_client PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIR}
    ${RapidJSON_INCLUDE_DIRS}
)
target_link_libraries(bitvavo_client PUBLIC
    connector_network
    ${Boost_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
)

# Executable target (links against the library)
add_executable(BitvavoConnector main.cpp)
target_link_libraries(BitvavoConnector PRIVATE bitvavo_client)
