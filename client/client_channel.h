#pragma once

#include <string>

#include "util/common.h"
#include "google/protobuf/service.h"

namespace xuanqiong {

class ClientChannel {
public:
    ClientChannel(Socket* socket);
    ~ClientChannel();

    template<typename Request, typename Response>
    void call_method(const Request* req, Response* resp);

private:
    Socket* socket_;

    DISALLOW_COPY_AND_ASSIGN(ClientChannel);
};

} // namespace xuanqiong
