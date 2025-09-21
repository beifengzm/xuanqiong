#pragma once

#include "net/accepter.h"

namespace xuanqiong {

class Scheduler;

struct RpcServerOptions {
    int port;
    int backlog;

    RpcServerOptions(int port, int backlog = 256)
        : port(port), backlog(backlog) {}
};

class RpcServer {
public:
    RpcServer(const RpcServerOptions& options);
    ~RpcServer() = default;

    void set_scheduler(Scheduler* scheduler) {
        scheduler_ = scheduler;
    }

    void start();

private:
    net::Accepter accepter_;
    Scheduler* scheduler_;

    RpcServer(const RpcServer&) = delete;
    RpcServer& operator=(const RpcServer&) = delete;
};

} // namespace xuanqiong
