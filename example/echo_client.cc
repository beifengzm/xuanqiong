#include <string_view>
#include <thread>

#include "example/echo.pb.h"
#include "util/common.h"
#include "util/iobuf.h"
#include "client/rpc_client.h"

using namespace xuanqiong;

int main() {
    RpcClient client("127.0.0.1", 8888);
    client.connect();

    while (true) {
        std::string_view data = "hello world";
        client.append(data.data(), data.size());
        int ret = client.send();
        info("send data: {}, ret: {}", data.data(), ret);

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
