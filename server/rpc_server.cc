#include <string.h>
#include <fcntl.h>
#include <google/protobuf/io/coded_stream.h>

#include "util/common.h"
#include "proto/message.pb.h"
#include "server/rpc_server.h"
#include "net/socket_utils.h"
#include "net/poll_connection.h"
#ifdef __linux__
#include "net/uring_connection.h"
#endif
#include "scheduler/scheduler.h"
#include "example/echo.pb.h"

namespace xuanqiong {

RpcServer::RpcServer(const RpcServerOptions& options)
    : options_(options), accepter_(options.port, options.backlog, options.nodelay) {
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
#ifdef __APPLE__
        auto conn = std::make_shared<net::PollConnection>(connfd, executor);
#else
        std::shared_ptr<net::Connection> conn;
        if (options_.sched_policy == SchedPolicy::POLL_POLICY) {
            conn = std::make_shared<net::PollConnection>(connfd, executor);
        } else {
            conn = std::make_shared<net::UringConnection>(connfd, executor);
        }
#endif
        executor->spawn([this, conn]() { send_fn(conn); });
        executor->spawn([this, conn]() { recv_fn(conn); });
    }
}

// coroutine function, one for each channel
Task RpcServer::recv_fn(std::shared_ptr<net::Connection> conn) {
    // register read event
    co_await RegisterReadAwaiter{conn.get()};

    while (true) {
        // deserialize message
        auto input_stream = conn->get_input_stream();

        // deserialize header
        // fetch header len
        while (conn->read_bytes() < sizeof(uint32_t) && !conn->closed()) {
            co_await conn->async_read();
        }
        if (conn->closed() && conn->read_bytes() < sizeof(uint32_t)) {
            break;
        }
        uint32_t header_len;
        // info("read {} bytes, read header sizeof(uint32_t) is 4", conn->read_bytes());
        if (!input_stream.fetch_uint32(&header_len)) {
            error("failed to read header len");
            break;
        }

        // read header
        while (conn->read_bytes() < header_len && !conn->closed()) {
            // info("read {} bytes, but header_len is {}", conn->read_bytes(), header_len);
            co_await conn->async_read();
        }
        if (conn->closed() && conn->read_bytes() < header_len) {
            break;
        }
        proto::Header header;
        input_stream.push_limit(header_len);
        if (!header.ParseFromZeroCopyStream(&input_stream)) {
            error("failed to parse header");
            break;
        }
        input_stream.pop_limit();
        // info("header: {}", header.DebugString());

        // check magic number && version
        if (header.magic() != MAGIC_NUM) {
            error("invalid magic number: 0x{:08x}", header.magic());
            break;
        }
        if (header.version() != VERSION) {
            error("invalid version: {}", header.version());
            break;
        }

         // deserialize request
         // fetch request len
        while (conn->read_bytes() < sizeof(uint32_t) && !conn->closed()) {
            co_await conn->async_read();
        }
        if (conn->closed() && conn->read_bytes() < sizeof(uint32_t)) {
            break;
        }
        uint32_t request_len;
        if (!input_stream.fetch_uint32(&request_len)) {
            error("failed to read request len");
            break;
        }

        const auto& service_name = header.service_name();
        const auto& method_name = header.method_name();

        auto iter = name2service_.find(service_name);
        if (iter == name2service_.end()) {
            error("service not found: {}", service_name);
            break;
        }
        auto service = iter->second;
        auto method = service->GetDescriptor()->FindMethodByName(method_name);
        if (method == nullptr) {
            error("method not found: {}", method_name);
            break;
        }

        std::unique_ptr<google::protobuf::Message> request(service->GetRequestPrototype(method).New());
        std::unique_ptr<google::protobuf::Message> response(service->GetResponsePrototype(method).New());

        // read request
        while (conn->read_bytes() < request_len) {
            co_await conn->async_read();
        }
        if (conn->closed() && conn->read_bytes() < request_len) {
            break;
        }
        input_stream.push_limit(request_len);
        if (!request->ParseFromZeroCopyStream(&input_stream)) {
            error("failed to parse request");
            break;
        }
        input_stream.pop_limit();
        // info("request: {}", request->DebugString());

        // call method
        service->CallMethod(method, nullptr, request.get(), response.get(), nullptr);

        // send response
        auto output_stream = conn->get_output_stream();
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

        conn->resume_write();
    }

    if (!conn->closed()) {
        conn->close();
    }
    info(
        "connection[{}] closed by peer: {}:{}",
        conn->fd(), conn->socket()->peer_addr(), conn->socket()->peer_port()
    );
    info("connection[{}] recv_fn done", conn->fd());
}

Task RpcServer::send_fn(std::shared_ptr<net::Connection> conn) {
    while (!conn->closed()) {
        co_await WaitWriteAwaiter{conn.get()};
        co_await conn->async_write();
    }
    info("connection[{}] send_fn done", conn->fd());
}

} // namespace xuanqiong
