cmake_minimum_required(VERSION 3.28)

project(xuanqiong)

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3 -Wall -Wextra -Wpedantic")

include_directories(${CMAKE_SOURCE_DIR})

set(GTEST_ROOT "${CMAKE_SOURCE_DIR}/third_party/gtest")
set(GTEST_INCLUDE_DIRS "${GTEST_ROOT}/include")
set(GTEST_LIBRARIES "${GTEST_ROOT}/lib")

include_directories(${GTEST_INCLUDE_DIRS})
# link_directories(${GTEST_LIBRARIES})

include_directories(${CMAKE_SOURCE_DIR}/third_party)

file(GLOB UTIL_SRCS "util/*.cc")
add_library(util STATIC ${UTIL_SRCS})
file(GLOB NET_SRCS "net/*.cc")
add_library(net STATIC ${NET_SRCS})
file(GLOB SCHED_SRCS "scheduler/*.cc")
add_library(sched STATIC ${SCHED_SRCS})
file(GLOB SERVER_SRCS "server/*.cc")
add_library(rpc_server STATIC ${SERVER_SRCS})
file(GLOB CLIENT_SRCS "client/*.cc")
add_library(rpc_client STATIC ${CLIENT_SRCS})

add_executable(echo_server "example/echo_server.cc")
target_link_libraries(echo_server rpc_server net util sched pthread)

add_executable(echo_client "example/echo_client.cc")
target_link_libraries(echo_client rpc_client net util sched pthread)
