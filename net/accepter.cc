#include <string>
#include <stdexcept>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>

#include "base/common.h"
#include "net/accepter.h"
#include "net/socket_utils.h"


namespace xuanqiong::net {

Accepter::Accepter(int port, int backlog)
    : sockfd_(-1), port_(port), backlog_(backlog) {

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
        close(sockfd_); 
    }
}

int Accepter::accept() {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd =
        SocketUtils::accept(sockfd_, (struct sockaddr*)&client_addr, &client_len);

    return client_fd;
}

} // namespace xuanqiong::net
