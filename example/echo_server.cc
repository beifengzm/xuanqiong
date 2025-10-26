#include "example/echo_service.h"
#include "server/rpc_server.h"

using namespace xuanqiong;

int main() {
    RpcServerOptions options(8888);
    RpcServer rpc_server(options);

    // register service
    EchoServiceImpl echo_service;
    rpc_server.register_service("EchoService", &echo_service);

    rpc_server.start();
    return 0;
}
