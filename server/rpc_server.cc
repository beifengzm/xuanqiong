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
    auto sched_options = SchedulerOptions(options.poll_timeout, options.sched_policy);
    scheduler_ = std::make_unique<Scheduler>(sched_options);
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
        auto socket = std::make_shared<net::Socket>(connfd, executor);
        executor->spawn(&RpcServer::send_fn, this, socket);
        executor->spawn(&RpcServer::recv_fn, this, socket);
    }
}

// coroutine function, one for each channel
Task RpcServer::recv_fn(std::shared_ptr<net::Socket> socket) {

    // register read event
    co_await RegisterReadAwaiter{socket.get()};

    while (true) {
        if (socket->closed()) {
            socket->resume_write();
            info("connection closed by peer: {}:{}", socket->peer_addr(), socket->peer_port());
            break;
        }

        while (socket->read_bytes() < sizeof(uint32_t)) {
            co_await socket->async_read();
        }

        // deserialize message
        auto input_stream = socket->get_input_stream();

        // deserialize header
        uint32_t header_len;
        info("read {} bytes, read header sizeof(uint32_t) is 4", socket->read_bytes());
        if (!input_stream.fetch_uint32(&header_len)) {
            error("failed to read header len");
            continue;
        }
        info("read {} bytes, header_len: {}", socket->read_bytes(), header_len);
        while (socket->read_bytes() < header_len) {
            info("read {} bytes, but header_len is {}", socket->read_bytes(), header_len);
            co_await socket->async_read();
        }

        proto::Header header;
        input_stream.push_limit(header_len);
        if (!header.ParseFromZeroCopyStream(&input_stream)) {
            error("failed to parse header");
            exit(1);
            continue;
        }
        input_stream.pop_limit();
        // info("header: {}", header.DebugString());

        // check magic number && version
        if (header.magic() != MAGIC_NUM) {
            error("invalid magic number: 0x{:08x}", header.magic());
            continue;
        }
        if (header.version() != VERSION) {
            error("invalid version: {}", header.version());
            continue;
        }

        // deserialize request
        uint32_t request_len;
        if (!input_stream.fetch_uint32(&request_len)) {
            error("failed to read request len");
            continue;
        }
        while (socket->read_bytes() < request_len) {
            co_await socket->async_read();
            info("read {} bytes, but request_len is {}", socket->read_bytes(), request_len);
        }

        // info("read {} bytes, request_len: {}", socket->read_bytes(), request_len);

        const auto& service_name = header.service_name();
        const auto& method_name = header.method_name();

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

        std::unique_ptr<google::protobuf::Message> request(service->GetRequestPrototype(method).New());
        std::unique_ptr<google::protobuf::Message> response(service->GetResponsePrototype(method).New());

        input_stream.push_limit(request_len);
        if (!request->ParseFromZeroCopyStream(&input_stream)) {
            error("failed to parse request");
            continue;
        }
        input_stream.pop_limit();
        // info("request: {}", request->DebugString());

        // call method
        service->CallMethod(method, nullptr, request.get(), response.get(), nullptr);

        // send response
        auto output_stream = socket->get_output_stream();
        // serialize header
        proto::Header resp_header;
        resp_header.set_magic(MAGIC_NUM);
        resp_header.set_version(VERSION);
        resp_header.set_message_type(proto::MessageType::RESPONSE);
        resp_header.set_request_id(header.request_id());
        uint32_t resp_header_len = resp_header.ByteSizeLong();
        output_stream.append(&resp_header_len, sizeof(resp_header_len));
        resp_header.SerializeToZeroCopyStream(&output_stream);

        // serialize response
        uint32_t response_len = response->ByteSizeLong();
        output_stream.append(&response_len, sizeof(response_len));
        response->SerializeToZeroCopyStream(&output_stream);

        socket->resume_write();
    }
}

Task RpcServer::send_fn(std::shared_ptr<net::Socket> socket) {
    while (!socket->closed()) {
        co_await WaitWriteAwaiter{socket.get()};
        co_await socket->async_write();
    }
}

} // namespace xuanqiong
