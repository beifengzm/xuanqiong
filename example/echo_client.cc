#include "client/rpc_client.h"

using namespace xuanqiong;

int main(int argc, char* argv[]) {
    RpcClient client("127.0.0.1", 8888);
    client.connect();
    return 0;
}
