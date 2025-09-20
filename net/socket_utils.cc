#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include "net/socket_utils.h"
#include "base/common.h"

namespace xuanqiong::net {

int SocketUtils::socket() {
    int sockfd;
    if ((sockfd = ::socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        error("create socket failed: {}", strerror(errno));
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

void SocketUtils::bind(int sockfd, const struct sockaddr* addr, socklen_t addrlen) {
    if (::bind(sockfd, addr, addrlen) < 0) {
        error("bind failed: {}", strerror(errno));
        close(sockfd);
        exit(EXIT_FAILURE);
    }
}

void SocketUtils::listen(int sockfd, int backlog) {
    if (::listen(sockfd, backlog) < 0) {
        error("listen failed: {}", strerror(errno));
        close(sockfd);
        exit(EXIT_FAILURE);
    }
}

int SocketUtils::accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen) {
    int client_fd;
    if ((client_fd = ::accept(sockfd, addr, addrlen)) < 0) {
        error("accept4 failed: {}", strerror(errno));
    }
    if (::fcntl(client_fd, F_SETFD, FD_CLOEXEC) == -1) {
        ::close(client_fd);
        return -1;
    }
    return client_fd;
}

void SocketUtils::inet_pton(int af, const char *src, void *dst) {
    if (::inet_pton(af, src, dst) <= 0) {
        error("inet_pton failed: {}", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

int SocketUtils::connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    int ret = ::connect(sockfd, addr, addrlen);
    if (ret < 0) {
        error("connect failed: {}", strerror(errno));
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    return ret;
}

void SocketUtils::setsocketopt(int sockfd, int level, int optname, const void* optval, socklen_t optlen) {
    if (::setsockopt(sockfd, level, optname, optval, optlen) < 0) {
        error("setsockopt failed: {}", strerror(errno));
        close(sockfd);
        exit(EXIT_FAILURE);
    }
}

} // namespace xuanqiong::net
