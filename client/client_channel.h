#pragma once

#include <string>
#include <memory>
#include <google/protobuf/service.h>

#include "net/socket.h"
#include "scheduler/task.h"
#include "util/output_stream.h"

namespace xuanqiong {

class Executor;

class ClientChannel : public google::protobuf::RpcChannel {
public:
    ClientChannel(const std::string& ip, int port, Executor* executor);

    void close();

    Task coro_fn();

    void CallMethod(
        const google::protobuf::MethodDescriptor* method,
        google::protobuf::RpcController* controller,
        const google::protobuf::Message* request,
        google::protobuf::Message* response,
        google::protobuf::Closure* done
    ) override;

private:
    std::unique_ptr<net::Socket> socket_;
    int64_t request_id_ = 0;

    DISALLOW_COPY_AND_ASSIGN(ClientChannel);
};

} // namespace xuanqiong
