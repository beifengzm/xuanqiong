#include <sys/socket.h>

#include "util/common.h"
#include "net/socket.h"
#include "net/socket_utils.h"

namespace xuanqiong::net {

Socket::Socket(int fd, Executor* executor) :
    sockfd_(fd),
    closed_(false),
    executor_(executor),
    read_handle_(nullptr),
    write_handle_(nullptr) {
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

ReadAwaiter Socket::async_read() {
    while (true) {
        int n = read_buf_.read_from(sockfd_);
        info("[async_read] read {} bytes", n);
        if (n > 0) {
            continue;
        } else if (n == 0) {
            close();
            return {this};
        } else {
            // retry
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                info("again, read data, errno: {}", errno);
                return {this};
            }
            if (errno == EINTR) {
                continue;
            }
            error("read data, errno: {}", errno);
            close();
            return {this};
        }
    }
}

WriteAwaiter Socket::async_write() {
    int write_remain = write_buf_.bytes();
    // info("[start] async_write, write_remain: {}", write_remain);
    while (write_remain > 0) {
        int n = write_buf_.write_to(sockfd_);
        if (n >= 0) {
            write_remain -= n;
            continue;
        } else {
            // retry
            if (errno == EINTR) {
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return {this, write_remain};
            }
            // other error
            error("write data, errno: {}", errno);
            return {this, -1};
        }
    }
    // info("[end] async_write done, write_remain: {}", write_remain);
    return {this, write_remain};
}

} // namespace xuanqiong::net
