#pragma once

#include <memory>
#include <queue>
#include <exception>
#include <google/protobuf/service.h>

#include "net/accepter.h"
#include "scheduler/task.h"
#include "scheduler/awaitable.h"

namespace xuanqiong {

namespace net {
class Connection;
}

struct RpcServerOptions {
    int port;
    int backlog;
    int nodelay;
    int poll_timeout;
    SchedPolicy sched_policy;

    RpcServerOptions(int port,
                     int backlog = 256,
                     bool nodelay = 1,
                     int poll_timeout = -1,
                     SchedPolicy policy = SchedPolicy::POLL_POLICY)
        : port(port), backlog(backlog), nodelay(nodelay), poll_timeout(poll_timeout), sched_policy(policy) {}
};

class RpcServer {
public:
    RpcServer(const RpcServerOptions& options);
    ~RpcServer() = default;

    void register_service(const std::string& service_name, google::protobuf::Service* service) {
        name2service_[service_name] = service;
    }

    void start();

private:
    Task recv_fn(std::shared_ptr<net::Connection> conn);
    Task send_fn(std::shared_ptr<net::Connection> conn);

    std::queue<std::shared_ptr<net::Connection>> send_queue_;

    net::Accepter accepter_;
    std::unique_ptr<Scheduler> scheduler_;

    std::unordered_map<std::string, google::protobuf::Service*> name2service_;

    RpcServer(const RpcServer&) = delete;
    RpcServer& operator=(const RpcServer&) = delete;
};

} // namespace xuanqiong
