#include <string_view>
#include <thread>

#include "example/echo.pb.h"
#include "example/echo.service.h"
#include "util/common.h"
#include "util/input_stream.h"
#include "util/output_stream.h"
#include "scheduler/scheduler.h"
#include "client/client_channel.h"

using namespace xuanqiong;

int main() {
    Scheduler scheduler(SchedPolicy::POLL_POLICY);
    auto executor = scheduler.alloc_executor();
    ClientChannel channel("127.0.0.1", 8888, executor);

    while (true) {
        std::string data(16, 'a');
        for (int i = 0; i < 1; ++i) {
            EchoRequest request;
            request.set_message(data.data());
            EchoServiceStub stub(&channel);
            EchoResponse response;
            stub.Echo1(&request, &response);
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}
