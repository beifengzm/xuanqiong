#pragma once

#include <string>

namespace xuanqiong {

class RpcClient {
public:
    RpcClient(const std::string& ip, int port);
    ~RpcClient() = default;

    int connect();

private:
    std::string ip_;
    int port_;
    int sockfd_;

    RpcClient(const RpcClient&) = delete;
    RpcClient& operator=(const RpcClient&) = delete;
};

} // namespace xuanqiong
