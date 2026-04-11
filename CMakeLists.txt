cmake_minimum_required(VERSION 3.15)
project(pgRouting-prototypes LANGUAGES CXX)

# Require C++17 across all targets
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Warnings for clean builds
add_compile_options(-Wall -Wextra -Wpedantic)

# Locate Boost (header-only graph library is sufficient)
find_package(Boost 1.40 REQUIRED)

if(NOT TARGET Boost::boost)
    add_library(Boost::boost INTERFACE IMPORTED)
    set_target_properties(Boost::boost PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${Boost_INCLUDE_DIRS}")
endif()

message(STATUS "Boost version: ${Boost_VERSION}")
message(STATUS "Boost include: ${Boost_INCLUDE_DIRS}")

# Target 1: Planar Face Traversal prototype
add_executable(planar_faces_prototype planar_faces/planar_faces_prototype.cpp)
target_link_libraries(planar_faces_prototype PRIVATE Boost::boost)

# Target 2: K-Core Decomposition prototype
add_executable(kcore_prototype kcore/kcore_prototype.cpp)
target_link_libraries(kcore_prototype PRIVATE Boost::boost)
