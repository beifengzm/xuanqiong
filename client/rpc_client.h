#pragma once

#include <string>

namespace xuanqiong {

namespace base {
class IOBuf;
}

class RpcClient {
public:
    RpcClient(const std::string& ip, int port);
    ~RpcClient() = default;

    void connect();

    int send(const util::IOBuf& iobuf);

private:
    std::string ip_;
    int port_;
    int sockfd_;

    RpcClient(const RpcClient&) = delete;
    RpcClient& operator=(const RpcClient&) = delete;
};

} // namespace xuanqiong
