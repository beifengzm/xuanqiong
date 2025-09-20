#include "client/rpc_client.h"
#include "net/socket_utils.h"

namespace xuanqiong {

RpcClient::RpcClient(const std::string& ip, int port) : ip_(ip), port_(port), sockfd_(-1) {
    
}

int RpcClient::connect() {
    sockfd_ = net::SocketUtils::socket();

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);

    net::SocketUtils::inet_pton(AF_INET, ip_.c_str(), &addr.sin_addr);
    net::SocketUtils::connect(sockfd_, (struct sockaddr*)&addr, sizeof(addr));

    return 0;
}

} // namespace xuanqiong
