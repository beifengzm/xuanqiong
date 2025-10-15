
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
        executor->spawn(coro_fn, connfd, executor);
    }
}

// coroutine function, one for each channel
Task<ServerPromise> RpcServer::coro_fn(int client_fd, Executor* executor) {
    auto socket = std::make_unique<net::Socket>(client_fd, executor);
    while (true) {
        bool closed = co_await socket->async_read();
        if (closed) {
            socket->close();
            info("connection closed by peer: {}:{}", socket->peer_addr(), socket->peer_port());
            break;
        }
        info("read {} bytes", socket->read_bytes());
    }
}

} // namespace xuanqiong
