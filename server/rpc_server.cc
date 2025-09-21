
#include <string.h>
#include <fcntl.h>

#include "util/common.h"
#include "server/rpc_server.h"
#include "net/socket_utils.h"
#include "server/server_task.h"

namespace xuanqiong {

RpcServer::RpcServer(const RpcServerOptions& options)
    : accepter_(options.port, options.backlog) {
}

void RpcServer::start() {
    while (true) {
        int connfd = accepter_.accept();
        if (connfd == -1) {
            // accept was interrupted by a signal — retry
            if (errno != EINTR) {
                error("error occurred in accept: %s", strerror(errno));;
            }
            continue;
        }

        struct sockaddr_in addr;
        socklen_t addrlen = sizeof(addr);
        getsockname(connfd, (struct sockaddr*)&addr, &addrlen);
        auto port = ntohs(addr.sin_port);
        info("accept new connection, ip: {} port: {}", inet_ntoa(addr.sin_addr), port);

        net::Socket socket(connfd);
        // handle_request(socket);
        // task.resume();
    }
}

} // namespace xuanqiong
