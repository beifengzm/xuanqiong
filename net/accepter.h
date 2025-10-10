#pragma once

#include <string>

#include "util/common.h"

namespace xuanqiong::net {

class Accepter {
public:
    Accepter(int port, int backlog, int nodelay);

    ~Accepter();

    int accept();

private:
    int sockfd_;
    int port_;           // listen port
    int backlog_;
    int nodelay_;

    void set_fd_param(int client_fd);

    DISALLOW_COPY_AND_ASSIGN(Accepter);
};

} // namespace xuanqiong::net
