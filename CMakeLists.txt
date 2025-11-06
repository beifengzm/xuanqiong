cmake_minimum_required(VERSION 3.28)
project(xuanqiong)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3 -Wall -Wextra \
    -Wno-unused-parameter \
    -Wno-deprecated-declarations")

# list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/third_party/protobuf")
find_package(Protobuf REQUIRED)
include_directories(${Protobuf_INCLUDE_DIRS})

function(gen_proto PROTO_DIR OUT_SRCS OUT_HDRS)
    file(GLOB PROTO_FILES "${CMAKE_SOURCE_DIR}/${PROTO_DIR}/*.proto")
    if(NOT PROTO_FILES)
        message(WARNING "No .proto files found in ${PROTO_DIR}")
        set(${OUT_SRCS} "" PARENT_SCOPE)
        set(${OUT_HDRS} "" PARENT_SCOPE)
        return()
    endif()

    set(GENERATED_SRCS "")
    set(GENERATED_HDRS "")

    foreach(PROTO_FILE ${PROTO_FILES})
        get_filename_component(FILE_NAME ${PROTO_FILE} NAME_WE)
        list(APPEND GENERATED_SRCS
            "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_DIR}/${FILE_NAME}.pb.cc"
        )
        list(APPEND GENERATED_HDRS
            "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_DIR}/${FILE_NAME}.pb.h"
            # "${CMAKE_CURRENT_BINARY_DIR}/${PROTO_DIR}/${FILE_NAME}.service.h"
        )
    endforeach()

    # set(PROTOC_PLUGIN_PATH "${CMAKE_CURRENT_BINARY_DIR}/protoc_plugin")

    add_custom_command(
        OUTPUT ${GENERATED_SRCS} ${GENERATED_HDRS}
        COMMAND ${Protobuf_PROTOC_EXECUTABLE}
        ARGS
            --proto_path=${CMAKE_CURRENT_SOURCE_DIR}
            --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
            # --myservice_out=${CMAKE_CURRENT_BINARY_DIR}
            # --plugin=protoc-gen-myservice=${PROTOC_PLUGIN_PATH}
            ${PROTO_FILES}
        DEPENDS ${PROTO_FILES}
        COMMENT "Generating service code from ${PROTO_FILES}"
        VERBATIM
    )

    set(${OUT_SRCS} ${GENERATED_SRCS} PARENT_SCOPE)
    set(${OUT_HDRS} ${GENERATED_HDRS} PARENT_SCOPE)
endfunction()

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(GTEST_ROOT "${CMAKE_SOURCE_DIR}/third_party/gtest")
if(EXISTS ${GTEST_ROOT})
    include_directories(${GTEST_ROOT}/include)
    link_directories(${GTEST_ROOT}/lib)
endif()

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

# file(GLOB ECHO_PROTO_FILES "${CMAKE_SOURCE_DIR}/example/*.proto")
# protobuf_generate_cpp(
#     ECHO_PROTO_SRCS ECHO_PROTO_HDRS
#     LANGUAGE cpp
#     PROTOS ${ECHO_PROTO_FILES}
# )
gen_proto(example ECHO_PROTO_SRCS ECHO_PROTO_HDRS)
add_library(echo_proto STATIC ${ECHO_PROTO_SRCS} ${ECHO_PROTO_HDRS})


# file(GLOB PROTO_FILES "${CMAKE_SOURCE_DIR}/proto/*.proto")
# protobuf_generate_cpp(
#     PROTO_SRCS PROTO_HDRS
#     LANGUAGE cpp
#     PROTOS ${PROTO_FILES}
# )
gen_proto(proto PROTO_SRCS PROTO_HDRS)
add_library(message_proto STATIC ${PROTO_SRCS} ${PROTO_HDRS})

add_executable(echo_server "example/echo_server.cc" "example/echo_service.cc")
target_link_libraries(
    echo_server
    rpc_server
    echo_proto
    message_proto
    net
    util
    sched
    pthread
    protobuf::libprotobuf
)

add_executable(echo_client "example/echo_client.cc")
target_link_libraries(
    echo_client
    rpc_client
    echo_proto
    message_proto
    net
    util
    sched
    pthread
    protobuf::libprotobuf
)

add_executable(rpc_press "press/rpc_press.cc")
target_link_libraries(
    rpc_press
    rpc_client
    echo_proto
    message_proto
    net
    util
    sched
    pthread
    protobuf::libprotobuf
)

add_executable(mpmc_queue_test "test/mpmc_queue_test.cc")
target_link_libraries(
    mpmc_queue_test
    util
    pthread
    gtest
    gtest_main
)
