#include <unistd.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <google/protobuf/io/coded_stream.h>

#include "util/common.h"
#include "net/socket_utils.h"
#include "client/client_channel.h"

namespace xuanqiong {

ClientChannel::ClientChannel(const std::string& ip, int port, Executor* executor) : executor_(executor) {
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
}

void ClientChannel::close() {
    socket_->close();
}

template<typename Request, typename Response>
Task<ClientPromise> ClientChannel::call_method(
    const Request* request,
    Response* response,
    const std::string& service_name,
    const std::string& method_name
) {
    // serialize request
    util::NetOutputStream output_stream = socket_->get_output_stream();
    google::protobuf::io::CodedOutputStream adaptor(&output_stream);
    request->SerializeToZeroCopyStream(&adaptor);

    co_await socket_->async_write();
    co_await socket_->async_read(response);
}

} // namespace xuanqiong
