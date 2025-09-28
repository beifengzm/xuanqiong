#pragma once

#include <memory>

#include "net/accepter.h"
#include "scheduler/task.h"
#include "scheduler/awaitable.h"

namespace xuanqiong {

class Scheduler;
class Executor;

struct ServerPromise {
    int client_fd;
    Executor* executor_;

    ServerPromise(int client_fd, Executor* executor)
        : client_fd(client_fd), executor_(executor) {}

    Task<ServerPromise> get_return_object() {
        return Task<ServerPromise>(std::coroutine_handle<ServerPromise>::from_promise(*this));
    }

    InitAwaiter initial_suspend() { return {client_fd, executor_}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void unhandled_exception() {
        std::terminate();
    }
    void return_void() {}
};

struct RpcServerOptions {
    int port;
    int backlog;
    int nodelay;
    SchedPolicy sched_policy;

    RpcServerOptions(int port,
                     int backlog = 256,
                     bool nodelay = 1,
                     SchedPolicy policy = SchedPolicy::EPOLL_POLICY)
        : port(port), backlog(backlog), nodelay(nodelay), sched_policy(policy) {}
};

class RpcServer {
public:
    RpcServer(const RpcServerOptions& options);
    ~RpcServer() = default;

    void start();

private:
    static Task<ServerPromise> coro_fn(int client_fd, Executor* executor);

    net::Accepter accepter_;
    std::unique_ptr<Scheduler> scheduler_;

    RpcServer(const RpcServer&) = delete;
    RpcServer& operator=(const RpcServer&) = delete;
};

} // namespace xuanqiong
