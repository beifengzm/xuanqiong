#pragma once

#include <string>

#include "util/output_stream.h"

namespace xuanqiong {

namespace util {
class OutputBuffer;
}

class RpcClient {
public:
    RpcClient(const std::string& ip, int port);
    ~RpcClient() = default;

    void connect();

    void close();

    int send();

    util::NetOutputStream get_output_stream() {
        return util::NetOutputStream(&output_buf_);
    }

private:
    std::string ip_;
    int port_;
    int sockfd_;
    util::OutputBuffer output_buf_;

    RpcClient(const RpcClient&) = delete;
    RpcClient& operator=(const RpcClient&) = delete;
};

} // namespace xuanqiong
