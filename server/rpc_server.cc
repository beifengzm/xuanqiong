
#include <string.h>
#include <fcntl.h>
#include <google/protobuf/io/coded_stream.h>

#include "util/common.h"
#include "proto/message.pb.h"
#include "server/rpc_server.h"
#include "net/socket_utils.h"
#include "net/socket.h"
#include "scheduler/scheduler.h"
#include "example/echo.pb.h"

namespace xuanqiong {

RpcServer::RpcServer(const RpcServerOptions& options)
    : accepter_(options.port, options.backlog, options.nodelay) {
    scheduler_ = std::make_unique<Scheduler>(options.sched_policy);
}

void RpcServer::start() {
    while (true) {
        int connfd = accepter_.accept();
        if (connfd == -1) {
            // accept was interrupted by a signal — retry
            if (errno != EINTR) {
                error("error occurred in accept: %s", strerror(errno));;
            }
            continue;
        }

        // launch a coroutine
        auto executor = scheduler_->alloc_executor();
        executor->spawn(&RpcServer::coro_fn, this, connfd, executor);
    }
}

// coroutine function, one for each channel
Task RpcServer::coro_fn(int fd, Executor* executor) {
    net::Socket socket(fd, executor);

    // register read event
    co_await RegisterReadAwaiter{&socket};

    while (true) {
        co_await socket.async_read();
        if (socket.closed()) {
            info("connection closed by peer: {}:{}", socket.peer_addr(), socket.peer_port());
            break;
        }

        // deserialize message
        auto input_stream = socket.get_input_stream();
        google::protobuf::io::CodedInputStream coded_input_stream(&input_stream);

        // deserialize header
        uint32_t header_len;
        if (!coded_input_stream.ReadVarint32(&header_len)) {
            error("failed to read header len");
            continue;
        }
        if (socket.read_bytes() < header_len) {
            continue;
        }

        auto limit = coded_input_stream.PushLimit(header_len);
        proto::Header header;
        if (!header.ParseFromCodedStream(&coded_input_stream)) {
            error("failed to parse header");
            continue;
        }
        coded_input_stream.PopLimit(limit);
        info("header: {}", header.DebugString());

        // deserialize request
        uint32_t request_len;
        if (!coded_input_stream.ReadVarint32(&request_len)) {
            error("failed to read request len");
            continue;
        }
        if (socket.read_bytes() < request_len) {
            continue;
        }

        auto service_name = header.service_name();
        auto method_name = header.method_name();

        auto iter = name2service_.find(service_name);
        if (iter == name2service_.end()) {
            error("service not found: {}", service_name);
            continue;
        }
        auto service = iter->second;
        auto method = service->GetDescriptor()->FindMethodByName(method_name);
        if (method == nullptr) {
            error("method not found: {}", method_name);
            continue;
        }

        std::unique_ptr<google::protobuf::Message> request;
        std::unique_ptr<google::protobuf::Message> response;
        request.reset(service->GetRequestPrototype(method).New());
        response.reset(service->GetResponsePrototype(method).New());

        limit = coded_input_stream.PushLimit(request_len);
        if (!request->ParseFromCodedStream(&coded_input_stream)) {
            error("failed to parse request");
            continue;
        }
        coded_input_stream.PopLimit(limit);

        // call method
        service->CallMethod(method, nullptr, request.get(), response.get(), nullptr);

        // send response
        auto output_stream = socket.get_output_stream();
        google::protobuf::io::CodedOutputStream coded_output_stream(&output_stream);
        // serialize header
        proto::Header resp_header;
        resp_header.set_magic(MAGIC_NUM);
        resp_header.set_version(1);
        resp_header.set_message_type(proto::MessageType::RESPONSE);
        resp_header.set_request_id(header.request_id());
        coded_output_stream.WriteVarint32(resp_header.ByteSizeLong());
        resp_header.SerializeToCodedStream(&coded_output_stream);

        // serialize response
        coded_output_stream.WriteVarint32(response->ByteSizeLong());
        response->SerializeToCodedStream(&coded_output_stream);

        co_await socket.async_write();
    }
}

} // namespace xuanqiong
