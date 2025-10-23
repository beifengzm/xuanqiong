#pragma once

#include <string>
#include <memory>
#include <google/protobuf/message.h>

#include "net/socket.h"
#include "scheduler/task.h"
#include "util/output_stream.h"

namespace xuanqiong {

class Executor;

class ClientChannel {
public:
    ClientChannel(const std::string& ip, int port, Executor* executor);

    void close();

    Task coro_fn();

    void call_method(const google::protobuf::Message* request,
                     google::protobuf::Message* response,
                     const std::string& service_name,
                     const std::string& method_name);

private:
    std::unique_ptr<net::Socket> socket_;

    DISALLOW_COPY_AND_ASSIGN(ClientChannel);
};

} // namespace xuanqiong
