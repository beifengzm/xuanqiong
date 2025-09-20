set(CMAKE_CXX_STANDARD 20)

file(GLOB SRCS "*.cc")

add_library(rpc_client STATIC ${SRCS})

include_directories(${GTEST_INCLUDE_DIRS})
link_directories(${GTEST_LIBRARIES})
