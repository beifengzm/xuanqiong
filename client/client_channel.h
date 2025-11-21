#pragma once

#include <string>
#include <memory>
#include <mutex>
#include <google/protobuf/service.h>

#include "scheduler/task.h"
#include "scheduler/scheduler.h"
#include "util/output_stream.h"

namespace xuanqiong {

class Executor;
namespace net {
class Connection;
}

struct ClientOptions {
    std::string ip;
    int port;
    SchedPolicy sched_policy = SchedPolicy::POLL_POLICY;
};

class ClientChannel : public google::protobuf::RpcChannel {
public:
    ClientChannel(const ClientOptions& options, Executor* executor);
    ~ClientChannel();

    void close();

    Task recv_fn();
    Task send_fn();

    void CallMethod(
        const google::protobuf::MethodDescriptor* method,
        google::protobuf::RpcController* controller,
        const google::protobuf::Message* request,
        google::protobuf::Message* response,
        google::protobuf::Closure* done
    ) override;

private:
    std::unique_ptr<net::Connection> conn_;
    using Session = std::pair<google::protobuf::Message*, google::protobuf::Closure*>;
    std::unordered_map<int64_t, Session> id2session_;

    Executor* executor_;

    int64_t request_id_ = 0;

    DISALLOW_COPY_AND_ASSIGN(ClientChannel);
};

} // namespace xuanqiong
