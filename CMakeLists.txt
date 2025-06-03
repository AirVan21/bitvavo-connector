cmake_minimum_required(VERSION 3.16)

# Set the project name and specify C++23 as the standard
project(BitvavoConnector LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find Boost library
find_package(Boost REQUIRED)

# Add the executable target
add_executable(BitvavoConnector main.cpp)

# Include Boost headers
if(Boost_FOUND)
    target_include_directories(BitvavoConnector PRIVATE ${Boost_INCLUDE_DIRS})
    target_link_libraries(BitvavoConnector PRIVATE ${Boost_LIBRARIES})
endif()
