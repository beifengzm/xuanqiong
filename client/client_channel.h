#pragma once

#include <string>
#include <memory>

#include "net/socket.h"
#include "scheduler/task.h"
#include "util/output_stream.h"

namespace xuanqiong {

class Executor;

class ClientChannel {
public:
    ClientChannel(const std::string& ip, int port, Executor* executor);

    void close();

    Task coro_fn(net::Socket* socket);

    template<typename Request, typename Response>
    void call_method(const Request* request,
                     Response* response,
                     const std::string& service_name,
                     const std::string& method_name);

private:
    std::unique_ptr<net::Socket> socket_;

    DISALLOW_COPY_AND_ASSIGN(ClientChannel);
};

template<typename Request, typename Response>
void ClientChannel::call_method(
    const Request* request,
    Response* response,
    const std::string& service_name,
    const std::string& method_name
) {
    // serialize request
    util::NetOutputStream output_stream = socket_->get_output_stream();
    // google::protobuf::io::CodedOutputStream coded_stream(&output_stream);
    request->SerializeToZeroCopyStream(&output_stream);

    auto executor = socket_->executor();
    executor->spawn(&ClientChannel::coro_fn, this, socket_.get());
}

} // namespace xuanqiong
