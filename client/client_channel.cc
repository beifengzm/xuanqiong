#include <unistd.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <google/protobuf/io/coded_stream.h>

#include "util/common.h"
#include "net/poll_connection.h"
#include "net/socket_utils.h"
#include "client/client_channel.h"
#include "proto/message.pb.h"

namespace xuanqiong {

ClientChannel::ClientChannel(const ClientOptions& options, Executor* executor)
    : executor_(executor) {
    int sockfd = net::SocketUtils::socket();

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(options.port);

    net::SocketUtils::inet_pton(AF_INET, options.ip.c_str(), &addr.sin_addr);
    net::SocketUtils::connect(sockfd, (struct sockaddr*)&addr, sizeof(addr));

    fcntl(sockfd, F_SETFL, O_NONBLOCK | O_CLOEXEC);

    if (options.sched_policy == SchedPolicy::POLL_POLICY) {
        conn_ = std::make_unique<net::PollConnection>(sockfd, executor);
    } else {
        // conn_ = std::make_unique<net::Socket>(sockfd, executor);
    }

    int sendbuf = 512 * 1024;
    net::SocketUtils::setsocketopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sendbuf, sizeof(sendbuf));

    int recvbuf = 512 * 1024;
    net::SocketUtils::setsocketopt(sockfd, SOL_SOCKET, SO_RCVBUF, &recvbuf, sizeof(recvbuf));

    // close nagle
    int nodelay = 1;
    net::SocketUtils::setsocketopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));

    // set close linger: default
    struct linger linger;
    linger.l_onoff = 0;
    linger.l_linger = 0;
    net::SocketUtils::setsocketopt(sockfd, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger));

    // start recv and send coroutine
    executor->spawn([this]() { send_fn(); });
    executor->spawn([this]() { recv_fn(); });
}

ClientChannel::~ClientChannel() = default;

void ClientChannel::close() {
    conn_->close();
}

Task ClientChannel::recv_fn() {
    // register read event
    co_await RegisterReadAwaiter{conn_.get()};

    while (true) {
        // deserialize message
        auto input_stream = conn_->get_input_stream();

        // deserialize header
        // fetch header len
        while (conn_->read_bytes() < sizeof(uint32_t) && !conn_->closed()) {
            co_await conn_->async_read();
        }
        if (conn_->closed() && conn_->read_bytes() < sizeof(uint32_t)) {
            break;
        }
        uint32_t header_len;
        // info("read {} bytes, sizeof(header_len) is 4", conn_->read_bytes());
        if (!input_stream.fetch_uint32(&header_len)) {
            error("failed to read header len");
            break;
        }

        // read header
        while (conn_->read_bytes() < header_len && !conn_->closed()) {
            co_await conn_->async_read();
            info("read {} bytes, but header_len is {}", conn_->read_bytes(), header_len);
        }
        if (conn_->closed() && conn_->read_bytes() < header_len) {
            break;
        }
        input_stream.push_limit(header_len);
        proto::Header header;
        if (!header.ParseFromZeroCopyStream(&input_stream)) {
            error("failed to parse header");
            break;
        }
        input_stream.pop_limit();
        // info("header: {}", header.DebugString());

        // deserialize request
        // fetch response len
        while (conn_->read_bytes() < sizeof(uint32_t) && !conn_->closed()) {
            co_await conn_->async_read();
        }
        if (conn_->closed() && conn_->read_bytes() < sizeof(uint32_t)) {
            break;
        }
        uint32_t response_len;
        if (!input_stream.fetch_uint32(&response_len)) {
            error("failed to read response len");
            continue;
        }

        // read response
        while (conn_->read_bytes() < response_len && !conn_->closed()) {
            co_await conn_->async_read();
            info("read {} bytes, but response_len is {}", conn_->read_bytes(), response_len);
        }
        if (conn_->closed() && conn_->read_bytes() < response_len) {
            break;
        }
        auto request_id = header.request_id();
        auto iter = id2session_.find(request_id);
        if (iter == id2session_.end()) {
            error("session not found: {}", request_id);
            continue;
        }
        auto [response, done] = iter->second;
        id2session_.erase(iter);

        input_stream.push_limit(response_len);
        if (!response->ParseFromZeroCopyStream(&input_stream)) {
            error("failed to parse response");
            continue;
        }
        input_stream.pop_limit();

        // handle response
        if (done) {
            done->Run();  // delete response in done
        } else {
            delete response;
        }
    }

    if (!conn_->closed()) {
        conn_->close();
    }

    info(
        "connection closed by peer: {}:{}",
        conn_->socket()->peer_addr(), conn_->socket()->peer_port()
    );
}

Task ClientChannel::send_fn() {
    while (!conn_->closed()) {
        // wait data for write ready
        co_await WaitWriteAwaiter{conn_.get()};
        co_await conn_->async_write();
    }
}

void ClientChannel::CallMethod(
    const google::protobuf::MethodDescriptor* method,
    google::protobuf::RpcController* controller,
    const google::protobuf::Message* request,
    google::protobuf::Message* response,
    google::protobuf::Closure* done
) {
    auto send_request = [=, this] {
        util::NetOutputStream output_stream = conn_->get_output_stream();

        // serialize header
        proto::Header header;
        header.set_magic(MAGIC_NUM);
        header.set_version(VERSION);
        header.set_message_type(proto::MessageType::REQUEST);
        header.set_request_id(request_id_++);
        header.set_service_name(method->service()->full_name());
        header.set_method_name(method->name());

        uint32_t header_len = header.ByteSizeLong();
        output_stream.append(&header_len, sizeof(header_len));
        header.SerializeToZeroCopyStream(&output_stream);

        // serialize request
        uint32_t request_len = request->ByteSizeLong();
        output_stream.append(&request_len, sizeof(request_len));
        request->SerializeToZeroCopyStream(&output_stream);

        delete controller;
        delete request;

        id2session_[header.request_id()] = {response, done};
        conn_->resume_write();
    };
    executor_->spawn(std::move(send_request));
}

} // namespace xuanqiong
