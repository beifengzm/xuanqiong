
#include <string.h>
#include <fcntl.h>

#include "util/common.h"
#include "server/rpc_server.h"
#include "net/socket_utils.h"
#include "net/channel.h"
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

// coroutine function
Task<ServerPromise> RpcServer::coro_fn(int client_fd, Executor* executor) {
    auto channel = std::make_unique<net::Channel>(client_fd, executor);
    while (true) {
        bool closed = co_await channel->async_read();
        if (closed) {
            info("connection closed by peer: {}:{}", channel->peer_addr(), channel->peer_port());
            break;
        }
        info("read {} bytes", channel->read_bytes());
    }
}

} // namespace xuanqiong
