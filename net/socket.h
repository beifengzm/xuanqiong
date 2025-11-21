#pragma once

#include <unistd.h>
#include <sys/socket.h>

#include "util/common.h"
#include "scheduler/awaitable.h"

namespace xuanqiong::net {

class Socket {
public:
    Socket(int fd);
    ~Socket();

    int fd() const { return sockfd_; }
    const std::string& local_addr() const { return local_addr_; }
    int local_port() const { return local_port_; }
    const std::string& peer_addr() const { return peer_addr_; }
    int peer_port() const { return peer_port_; }

    // close socket
    void close() {
        if (closed_) { return; }
        shutdown(sockfd_, SHUT_WR);
        closed_ = true;
    }

    bool closed() const { return closed_; }

private:
    int sockfd_;              // peer socket

    std::string local_addr_;  // local address
    int local_port_;          // local port
    std::string peer_addr_;   // peer address
    int peer_port_;           // peer port

    bool closed_;             // socket closed
};

} // namespace xuanqiong::net
