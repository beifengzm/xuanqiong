#pragma once

#include <string>

namespace xuanqiong::net {

class Accepter {
public:
    Accepter(int port, int backlog = 256);

    ~Accepter();

    int accept();

private:
    int sockfd_;
    int port_;           // listen port
    int backlog_;

    void set_fd_param(int client_fd);

    Accepter(const Accepter&) = delete;
    Accepter& operator=(const Accepter&) = delete;
};

} // namespace xuanqiong::net
