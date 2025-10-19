#include <string_view>
#include <thread>

#include "example/echo.pb.h"
#include "util/common.h"
#include "util/input_stream.h"
#include "util/output_stream.h"
#include "scheduler/scheduler.h"
#include "client/client_channel.h"

using namespace xuanqiong;

void coro_fn(ClientChannel& client) {
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
}

int main() {
    Scheduler scheduler(SchedPolicy::POLL_POLICY);
    auto executor = scheduler.alloc_executor();
    ClientChannel client("127.0.0.1", 8888, executor);

    executor->spawn(coro_fn, client);

    // while (true) {
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
