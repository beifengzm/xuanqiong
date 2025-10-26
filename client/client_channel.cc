#include <unistd.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <google/protobuf/io/coded_stream.h>

#include "util/common.h"
#include "net/socket_utils.h"
#include "client/client_channel.h"
#include "proto/message.pb.h"

namespace xuanqiong {

ClientChannel::ClientChannel(const std::string& ip, int port, Executor* executor) {
    int sockfd = net::SocketUtils::socket();

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    net::SocketUtils::inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    net::SocketUtils::connect(sockfd, (struct sockaddr*)&addr, sizeof(addr));

    fcntl(sockfd, F_SETFL, O_NONBLOCK | O_CLOEXEC);

    socket_ = std::make_unique<net::Socket>(sockfd, executor);

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
    executor->spawn(&ClientChannel::recv_fn, this);
    executor->spawn(&ClientChannel::send_fn, this);
}

void ClientChannel::close() {
    socket_->close();
}

Task ClientChannel::recv_fn() {
    // register read event
    co_await RegisterReadAwaiter{socket_.get()};

    while (true) {
        co_await socket_->async_read();
        if (socket_->closed()) {
            socket_->resume_write();
            info("connection closed by peer: {}:{}", socket_->peer_addr(), socket_->peer_port());
            break;
        }

        // deserialize message
        auto input_stream = socket_->get_input_stream();
        google::protobuf::io::CodedInputStream coded_input_stream(&input_stream);

        // deserialize header
        uint32_t header_len;
        if (!coded_input_stream.ReadVarint32(&header_len)) {
            error("failed to read header len");
            continue;
        }
        if (socket_->read_bytes() < header_len) {
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
        uint32_t response_len;
        if (!coded_input_stream.ReadVarint32(&response_len)) {
            error("failed to read response len");
            continue;
        }
        if (socket_->read_bytes() < response_len) {
            continue;
        }

        auto iter = id2session_.find(header.request_id());
        if (iter == id2session_.end()) {
            error("session not found: {}", header.request_id());
            continue;
        }
        auto [response, done] = iter->second;
        id2session_.erase(iter);

        limit = coded_input_stream.PushLimit(response_len);
        if (!response->ParseFromCodedStream(&coded_input_stream)) {
            error("failed to parse response");
            continue;
        }
        coded_input_stream.PopLimit(limit);

        done->Run();
    }
}


Task ClientChannel::send_fn() {
    while (!socket_->closed()) {
        co_await WaitWriteAwaiter{socket_.get()};
        co_await socket_->async_write();
    }
}

void ClientChannel::CallMethod(
    const google::protobuf::MethodDescriptor* method,
    google::protobuf::RpcController* controller,
    const google::protobuf::Message* request,
    google::protobuf::Message* response,
    google::protobuf::Closure* done
) {
    util::NetOutputStream output_stream = socket_->get_output_stream();
    google::protobuf::io::CodedOutputStream coded_stream(&output_stream);

    // serialize header
    proto::Header header;
    header.set_magic(MAGIC_NUM);
    header.set_version(VERSION);
    header.set_message_type(proto::MessageType::REQUEST);
    header.set_request_id(request_id_++);
    header.set_service_name(method->service()->full_name());
    header.set_method_name(method->name());
    info("header len: {}", header.ByteSizeLong());
    coded_stream.WriteVarint32(header.ByteSizeLong());
    header.SerializeToCodedStream(&coded_stream);

    // serialize request
    coded_stream.WriteVarint32(request->ByteSizeLong());
    request->SerializeToCodedStream(&coded_stream);

    // store response
    id2session_[header.request_id()] = {response, done};

    socket_->resume_write();
}

} // namespace xuanqiong
