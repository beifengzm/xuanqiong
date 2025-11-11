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

void handle_response(EchoResponse* response) {
    info("response: {}", response->DebugString());
    delete response;
}

int main() {
    SchedulerOptions sched_options(1000, SchedPolicy::POLL_POLICY);
    Scheduler scheduler(sched_options);
    auto executor = scheduler.alloc_executor();
    ClientChannel channel("127.0.0.1", 8888, executor);

    std::string data("echo request");
    for (int i = 0; i <= 1000; ++i) {
        EchoService_Stub stub(&channel);
        auto request = new EchoRequest;
        request->set_message(data.data() + std::to_string(i));
        auto response = new EchoResponse;
        auto controller = new RpcController;
        auto done = google::protobuf::NewCallback(handle_response, response);
        stub.Echo1(controller, request, response, done);
        // std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    std::this_thread::sleep_for(std::chrono::seconds(5));
    scheduler.stop();
}
