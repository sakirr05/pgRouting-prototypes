# pgRouting Algorithm Prototypes - CMake build
# Requires Boost 1.76+ (max_weighted_match needs it). C++17.

cmake_minimum_required(VERSION 3.15)
project(pgRouting-prototypes LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compile options: clean build with -Wall -Wextra
add_compile_options(-Wall -Wextra)

# Find Boost (1.38+ for stoer_wagner/planar_faces; 1.76+ needed for max_weighted_match)
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
find_package(Boost 1.38)

# Fallback: if find_package did not find Boost, try common install locations
if(NOT Boost_FOUND)
  set(BOOST_FALLBACK_PATHS
    /usr/include
    /usr/local/include
    /opt/homebrew/include
    "C:/local/boost"
  )
  foreach(PATH ${BOOST_FALLBACK_PATHS})
    if(EXISTS "${PATH}/boost/graph/adjacency_list.hpp")
      set(Boost_INCLUDE_DIRS "${PATH}")
      set(Boost_FOUND TRUE)
      # Try to get version from boost/version.hpp
      if(EXISTS "${PATH}/boost/version.hpp")
        file(STRINGS "${PATH}/boost/version.hpp" BOOST_VERSION_LINE REGEX "^#define BOOST_VERSION ")
        string(REGEX REPLACE "^#define BOOST_VERSION +([0-9]+).*" "\\1" Boost_VERSION "${BOOST_VERSION_LINE}")
        if(Boost_VERSION)
          math(EXPR Boost_VERSION_MAJOR "${Boost_VERSION} / 100000")
          math(EXPR Boost_VERSION_MINOR "${Boost_VERSION} / 100 % 1000")
          math(EXPR Boost_VERSION_PATCH "${Boost_VERSION} % 100")
          set(Boost_VERSION "${Boost_VERSION_MAJOR}.${Boost_VERSION_MINOR}.${Boost_VERSION_PATCH}")
        endif()
      endif()
      break()
    endif()
  endforeach()
endif()

if(Boost_FOUND)
  message(STATUS "Boost found at: ${Boost_INCLUDE_DIRS}")
  message(STATUS "Boost version: ${Boost_VERSION}")
  # If we used fallback, Boost::boost target may not exist; create an interface library for includes
  if(NOT TARGET Boost::boost)
    add_library(Boost::boost INTERFACE IMPORTED)
    set_target_properties(Boost::boost PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${Boost_INCLUDE_DIRS}")
  endif()
else()
  message(FATAL_ERROR
    "Boost not found. Please install Boost (1.76+ required for max_weighted_match):\n"
    "  Ubuntu:    sudo apt install libboost-all-dev\n"
    "  macOS:     brew install boost\n"
    "  Windows:   choco install boost-msvc-14.3\n"
    "Or set BOOST_ROOT: cmake .. -DBOOST_ROOT=/path/to/boost")
endif()

# ---- Stoer-Wagner ----
add_executable(stoer_wagner_prototype stoer_wagner/stoer_wagner_prototype.cpp)
target_include_directories(stoer_wagner_prototype PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(stoer_wagner_prototype PRIVATE Boost::boost)

add_executable(test_stoer_wagner stoer_wagner/test_stoer_wagner.cpp)
target_include_directories(test_stoer_wagner PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(test_stoer_wagner PRIVATE Boost::boost)

# ---- Planar faces ----
add_executable(planar_faces_prototype planar_faces/planar_faces_prototype.cpp)
target_include_directories(planar_faces_prototype PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(planar_faces_prototype PRIVATE Boost::boost)

add_executable(test_planar_faces planar_faces/test_planar_faces.cpp)
target_include_directories(test_planar_faces PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(test_planar_faces PRIVATE Boost::boost)

# ---- Max weighted matching ----
add_executable(max_weighted_match_prototype max_weighted_match/max_weighted_match_prototype.cpp)
target_include_directories(max_weighted_match_prototype PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(max_weighted_match_prototype PRIVATE Boost::boost)

add_executable(test_max_weighted_match max_weighted_match/test_max_weighted_match.cpp)
target_include_directories(test_max_weighted_match PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(test_max_weighted_match PRIVATE Boost::boost)

# Custom target: run all three test executables sequentially, capture exit codes, print summary
add_custom_target(test
  COMMAND ${CMAKE_COMMAND}
    -DTEST1_EXE=$<TARGET_FILE:test_stoer_wagner>
    -DTEST2_EXE=$<TARGET_FILE:test_planar_faces>
    -DTEST3_EXE=$<TARGET_FILE:test_max_weighted_match>
    -P ${CMAKE_SOURCE_DIR}/run_tests.cmake
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  DEPENDS test_stoer_wagner test_planar_faces test_max_weighted_match
  COMMENT "Running all prototype tests"
)