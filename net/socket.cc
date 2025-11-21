#include <sys/socket.h>

#include "util/common.h"
#include "net/socket.h"
#include "net/socket_utils.h"

namespace xuanqiong::net {

Socket::Socket(int fd) : sockfd_(fd), closed_(false) {
    // get local address
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    getsockname(fd, (struct sockaddr*)&addr, &addrlen);
    local_addr_ = SocketUtils::inet_ntoa(addr.sin_addr);
    local_port_ = ntohs(addr.sin_port);

    // get peer address
    struct sockaddr_in peer_addr;
    socklen_t peer_addrlen = sizeof(peer_addr);
    getpeername(fd, (struct sockaddr*)&peer_addr, &peer_addrlen);
    peer_addr_ = SocketUtils::inet_ntoa(peer_addr.sin_addr);
    peer_port_ = ntohs(peer_addr.sin_port);

    info("connection: local: {}:{} <-> peer: {}:{}",
        local_addr_, local_port_, peer_addr_, peer_port_);
}

Socket::~Socket() {
    info("close socket: {}", sockfd_);
    ::close(sockfd_);
}

} // namespace xuanqiong::net
