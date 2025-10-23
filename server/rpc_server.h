#pragma once

#include <memory>
#include <exception>

#include "net/accepter.h"
#include "scheduler/task.h"
#include "scheduler/awaitable.h"

namespace xuanqiong {

struct RpcServerOptions {
    int port;
    int backlog;
    int nodelay;
    SchedPolicy sched_policy;

    RpcServerOptions(int port,
                     int backlog = 256,
                     bool nodelay = 1,
                     SchedPolicy policy = SchedPolicy::POLL_POLICY)
        : port(port), backlog(backlog), nodelay(nodelay), sched_policy(policy) {}
};

class RpcServer {
public:
    RpcServer(const RpcServerOptions& options);
    ~RpcServer() = default;

    void start();

private:
    Task coro_fn(int fd, Executor* executor);

    net::Accepter accepter_;
    std::unique_ptr<Scheduler> scheduler_;

    RpcServer(const RpcServer&) = delete;
    RpcServer& operator=(const RpcServer&) = delete;
};

} // namespace xuanqiong
