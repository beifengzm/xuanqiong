#include <string_view>
#include <thread>

#include "example/echo.pb.h"
#include "util/common.h"
#include "util/input_stream.h"
#include "util/output_stream.h"
#include "client/rpc_client.h"

using namespace xuanqiong;

int main() {
    RpcClient client("127.0.0.1", 8888);
    client.connect();

    while (true) {
        std::string data(16, 'a');
        auto output_stream = client.get_output_stream();
        for (int i = 0; i < 100; ++i) {
            EchoRequest request;
            request.set_message(data.data());
            request.SerializeToZeroCopyStream(&output_stream);
            client.send();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    return 0;
}
