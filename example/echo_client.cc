#include <string_view>
#include <thread>

#include "example/echo.pb.h"
#include "util/common.h"
#include "util/input_stream.h"
#include "util/output_stream.h"
#include "util/service.h"
#include "scheduler/scheduler.h"
#include "client/client_channel.h"

using namespace xuanqiong;

int main() {
    Scheduler scheduler(SchedPolicy::POLL_POLICY);
    auto executor = scheduler.alloc_executor();
    ClientChannel channel("127.0.0.1", 8888, executor);

    std::string data("echo request");
    for (int i = 0; i < 1000; ++i) {
        EchoRequest request;
        request.set_message(data.data() + std::to_string(i));
        EchoService_Stub stub(&channel);
        auto response = new EchoResponse;
        RpcController controller;
        stub.Echo(&controller, &request, response, nullptr);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    std::this_thread::sleep_for(std::chrono::seconds(2));
}
