cmake_minimum_required(VERSION 2.6)
project(basic_test)

################################
# GTest
################################
ADD_SUBDIRECTORY (gtest-1.7.0)
enable_testing()
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR} ../include ../lib)
set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} ../lib)
FIND_LIBRARY( GRAPH_LIB NAMES graph
        PATHS ../lib/ )

################################
# Unit Tests
################################
# Add test cpp file
#add_executable( runUnitTests testgtest.cpp vertex.cpp prioritaryQueue.cpp graph.cpp montecarlo.cpp shortpath.cpp spanningTree.cpp )
add_executable( runUnitTests testgtest.cpp prioritaryQueue.cpp vertex.cpp )
add_dependencies( runUnitTests googletest )
# Link test executable against gtest & gtest_main
#target_link_libraries(runUnitTests gtest gtest_main Graph)
target_link_libraries(runUnitTests gtest gtest_main )
add_test( runUnitTests runUnitTests )
