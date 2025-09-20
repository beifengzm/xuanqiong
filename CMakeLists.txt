cmake_minimum_required(VERSION 3.28)

project(xuanqiong)

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3")

include_directories(${CMAKE_SOURCE_DIR})

set(GTEST_ROOT "${CMAKE_SOURCE_DIR}/third_party/gtest")
set(GTEST_INCLUDE_DIRS "${GTEST_ROOT}/include")
set(GTEST_LIBRARIES "${GTEST_ROOT}/lib")

include_directories(${GTEST_INCLUDE_DIRS})

include_directories(${CMAKE_SOURCE_DIR}/third_party)

add_subdirectory(net)
add_subdirectory(server)
add_subdirectory(client)
add_subdirectory(example)
