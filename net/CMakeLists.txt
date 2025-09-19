set(CMAKE_CXX_STANDARD 20)

file(GLOB CORE_SRCS "*.cc")

add_library(net STATIC ${CORE_SRCS})

include_directories(${GTEST_INCLUDE_DIRS})
link_directories(${GTEST_LIBRARIES})
