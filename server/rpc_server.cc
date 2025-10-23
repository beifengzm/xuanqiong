
#include <string.h>
#include <fcntl.h>

#include "util/common.h"
#include "server/rpc_server.h"
#include "net/socket_utils.h"
#include "net/socket.h"
#include "scheduler/scheduler.h"

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

        info("read {} bytes", socket.read_bytes());
    }
}

} // namespace xuanqiong
