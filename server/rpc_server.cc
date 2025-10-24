
#include <string.h>
#include <fcntl.h>
#include <google/protobuf/io/coded_stream.h>

#include "util/common.h"
#include "proto/message.pb.h"
#include "server/rpc_server.h"
#include "net/socket_utils.h"
#include "net/socket.h"
#include "scheduler/scheduler.h"
#include "example/echo.pb.h"

namespace xuanqiong {

RpcServer::RpcServer(const RpcServerOptions& options)
    : accepter_(options.port, options.backlog, options.nodelay) {
    scheduler_ = std::make_unique<Scheduler>(options.sched_policy);
}

void RpcServer::start() {
    while (true) {
        int connfd = accepter_.accept();
        if (connfd == -1) {
            // accept was interrupted by a signal — retry
            if (errno != EINTR) {
                error("error occurred in accept: %s", strerror(errno));;
            }
            continue;
        }

        // launch a coroutine
        auto executor = scheduler_->alloc_executor();
        executor->spawn(&RpcServer::coro_fn, this, connfd, executor);
    }
}

// coroutine function, one for each channel
Task RpcServer::coro_fn(int fd, Executor* executor) {
    net::Socket socket(fd, executor);

    // register read event
    co_await RegisterReadAwaiter{&socket};

    while (true) {
        co_await socket.async_read();
        if (socket.closed()) {
            info("connection closed by peer: {}:{}", socket.peer_addr(), socket.peer_port());
            break;
        }

        // deserialize message
        auto input_stream = socket.get_input_stream();
        google::protobuf::io::CodedInputStream coded_stream(&input_stream);

        // deserialize header
        uint32_t header_len;
        if (!coded_stream.ReadVarint32(&header_len)) {
            error("failed to read header len");
            continue;
        }
        if (socket.read_bytes() < header_len) {
            continue;
        }

        info("header len: {}", header_len);
        auto limit = coded_stream.PushLimit(header_len);
        proto::Header header;
        if (!header.ParseFromCodedStream(&coded_stream)) {
            error("failed to parse header");
            continue;
        }
        coded_stream.PopLimit(limit);
        info("header: {}", header.DebugString());

        // deserialize request
        uint32_t request_len;
        if (!coded_stream.ReadVarint32(&request_len)) {
            error("failed to read request len");
            continue;
        }
        if (socket.read_bytes() < request_len) {
            continue;
        }

        info("request len: {}", request_len);
        limit = coded_stream.PushLimit(request_len);
        EchoRequest request;
        if (!request.ParseFromCodedStream(&coded_stream)) {
            error("failed to parse request");
            continue;
        }
        coded_stream.PopLimit(limit);
        info("request: {}", request.DebugString());
    }
}

} // namespace xuanqiong
