#pragma once

#include <memory>
#include <exception>

#include "net/accepter.h"
#include "scheduler/task.h"
#include "scheduler/awaitable.h"

namespace xuanqiong {

struct ServerPromise {
    net::Socket* socket;

    ServerPromise(std::unique_ptr<net::Socket>&& socket) : socket(socket.get()) {}

    Task<ServerPromise> get_return_object() {
        return Task<ServerPromise>(std::coroutine_handle<ServerPromise>::from_promise(*this));
    }

    InitAwaiter initial_suspend() { return {socket}; }
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
                     SchedPolicy policy = SchedPolicy::POLL_POLICY)
        : port(port), backlog(backlog), nodelay(nodelay), sched_policy(policy) {}
};

class RpcServer {
public:
    RpcServer(const RpcServerOptions& options);
    ~RpcServer() = default;

    void start();

private:
    static Task<ServerPromise> coro_fn(std::unique_ptr<net::Socket>&& socket);

    net::Accepter accepter_;
    std::unique_ptr<Scheduler> scheduler_;

    RpcServer(const RpcServer&) = delete;
    RpcServer& operator=(const RpcServer&) = delete;
};

} // namespace xuanqiong
