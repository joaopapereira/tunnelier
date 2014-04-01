cmake_minimum_required(VERSION 2.6)
project(basic_test)

################################
# GTest
################################
ADD_SUBDIRECTORY (gtest)
enable_testing()
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR} ../include ../lib)
set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} ../lib)

################################
# Unit Tests
################################
# Add test cpp file
add_executable( runUnitTests sharedMemory.cpp)
add_dependencies( runUnitTests googletest )
# Link test executable against gtest & gtest_main
#target_link_libraries(runUnitTests gtest gtest_main Graph)
target_link_libraries(runUnitTests gtest gtest_main )
add_test( runUnitTests runUnitTests )
