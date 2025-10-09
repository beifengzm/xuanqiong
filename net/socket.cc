#include <sys/socket.h>

#include "util/common.h"
#include "net/socket.h"
#include "net/socket_utils.h"

namespace xuanqiong::net {

Socket::Socket(int fd, Executor* executor)
    : sockfd_(fd), executor_(executor) {
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

    info("new connection, local: {}:{} <-> peer: {}:{}",
        local_addr_, local_port_, peer_addr_, peer_port_);
}

Socket::~Socket() {
    ::close(sockfd_);
}

ReadAwaiter Socket::async_read() {
    while (true) {
        int n = read_buf_.read_from(sockfd_);
        if (n > 0) {
            continue;
        } else if (n == 0) {
            return {sockfd_, true, executor_};
        } else {
            // retry
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return {sockfd_, false, executor_};
            }
            if (errno == EINTR) {
                continue;
            }
            error("read data, errno: {}", errno);
            return {sockfd_, true, executor_};
        }
    }
}

} // namespace xuanqiong::net
