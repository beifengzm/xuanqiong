#pragma once

#include "net/accepter.h"

namespace xuanqiong {

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

    void start();

private:
    net::Accepter accepter_;

    RpcServer(const RpcServer&) = delete;
    RpcServer& operator=(const RpcServer&) = delete;
};

} // namespace xuanqiong
