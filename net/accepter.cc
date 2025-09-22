#include <string>
#include <stdexcept>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>

#include "util/common.h"
#include "net/accepter.h"
#include "net/socket_utils.h"

namespace xuanqiong::net {

Accepter::Accepter(int port, int backlog, int nodelay)
    : sockfd_(-1), port_(port), backlog_(backlog), nodelay_(nodelay) {

    // create socket
    sockfd_ = SocketUtils::socket();

    int opt = 1;
    SocketUtils::setsocketopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    SocketUtils::bind(sockfd_, (struct sockaddr*)&server_addr, sizeof(server_addr));

    // start listen
    info("Accepter start listen on port {}", port_);
    SocketUtils::listen(sockfd_, backlog_);
}

Accepter::~Accepter() {
    if (sockfd_ != -1) {
        ::close(sockfd_);
    }
}

void Accepter::set_fd_param(int client_fd) {
    // set socket to non-blocking
    if (::fcntl(client_fd, F_SETFL, O_NONBLOCK | O_CLOEXEC) == -1) {
        error("fcntl failed: {}", strerror(errno));
        ::close(client_fd);
    }

    // send buffer size
    int sendbuf = 512 * 1024;
    SocketUtils::setsocketopt(client_fd, SOL_SOCKET, SO_SNDBUF, &sendbuf, sizeof(sendbuf));

    // recv buffer size
    int recvbuf = 512 * 1024;
    SocketUtils::setsocketopt(client_fd, SOL_SOCKET, SO_RCVBUF, &recvbuf, sizeof(recvbuf));

    // set keepalive
    int keepalive = 1;
    SocketUtils::setsocketopt(client_fd, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive));

    // set nodelay
    SocketUtils::setsocketopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &nodelay_, sizeof(nodelay_));
}

int Accepter::accept() {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd =
        SocketUtils::accept(sockfd_, (struct sockaddr*)&client_addr, &client_len);

    if (client_fd != -1) {
        set_fd_param(client_fd);
    }

    return client_fd;
}

} // namespace xuanqiong::net
