#include <unistd.h>
#include <netinet/tcp.h>

#include "util/common.h"
#include "util/iobuf.h"
#include "client/rpc_client.h"
#include "net/socket_utils.h"

namespace xuanqiong {

RpcClient::RpcClient(const std::string& ip, int port) : ip_(ip), port_(port), sockfd_(-1) {}

void RpcClient::connect() {
    sockfd_ = net::SocketUtils::socket();

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);

    net::SocketUtils::inet_pton(AF_INET, ip_.c_str(), &addr.sin_addr);
    net::SocketUtils::connect(sockfd_, (struct sockaddr*)&addr, sizeof(addr));

    int sendbuf = 512 * 1024;
    net::SocketUtils::setsocketopt(sockfd_, SOL_SOCKET, SO_SNDBUF, &sendbuf, sizeof(sendbuf));

    int recvbuf = 512 * 1024;
    net::SocketUtils::setsocketopt(sockfd_, SOL_SOCKET, SO_RCVBUF, &recvbuf, sizeof(recvbuf));

    // close nagle
    int nodelay = 1;
    net::SocketUtils::setsocketopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &nodelay, sizeof(nodelay));

    // set close linger: default
    struct linger linger;
    linger.l_onoff = 0;
    linger.l_linger = 0;
    net::SocketUtils::setsocketopt(sockfd_, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger));
}

void RpcClient::close() {
    if (sockfd_ >= 0) {
        ::close(sockfd_);
        sockfd_ = -1;
    }
}

int RpcClient::append(const char* data, size_t size) {
    iobuf_.append(data, size);
    return size;
}

int RpcClient::send() {
    return iobuf_.write_to(sockfd_);
}

} // namespace xuanqiong
