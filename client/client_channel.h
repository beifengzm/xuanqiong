#pragma once

#include <string>

#include "util/common.h"
#include "google/protobuf/service.h"

namespace xuanqiong {

namespace net {
class Socket;
}

class ClientChannel {
public:
    ClientChannel(net::Socket* socket) : socket_(socket) {}
    ~ClientChannel() = default;

    template<typename Request, typename Response>
    void call_method(const Request* req, Response* resp);

private:
    net::Socket* socket_;

    DISALLOW_COPY_AND_ASSIGN(ClientChannel);
};

} // namespace xuanqiong
