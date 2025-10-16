#pragma once

#include <string>
#include <memory>

#include "net/socket.h"
#include "util/output_stream.h"

namespace xuanqiong {

class Executor;

class ClientChannel {
public:
    ClientChannel(const std::string& ip, int port, Executor* executor);

    void close();

    int send();

    util::NetOutputStream get_output_stream() {
        return socket_->get_output_stream();
    }

    template<typename Request, typename Response>
    void call_method(const Request* request,
                     Response* response,
                     const std::string& service_name,
                     const std::string& method_name);

private:
    std::unique_ptr<net::Socket> socket_;

    DISALLOW_COPY_AND_ASSIGN(ClientChannel);
};

} // namespace xuanqiong
