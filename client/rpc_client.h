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

    void close();

    int append(const char* data, size_t size);

    int send();

private:
    std::string ip_;
    int port_;
    int sockfd_;
    util::IOBuf iobuf_;

    RpcClient(const RpcClient&) = delete;
    RpcClient& operator=(const RpcClient&) = delete;
};

} // namespace xuanqiong
