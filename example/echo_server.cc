#include "server/rpc_server.h"

using namespace xuanqiong;

int main(int argc, char* argv[]) {
    RpcServerOptions options(8888);
    RpcServer rpc_server(options);
    rpc_server.start();
    return 0;
}