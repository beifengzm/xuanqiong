#include <atomic>
#include <chrono>
#include <thread>
#include <vector>
#include <mutex>
#include <algorithm>
#include <iostream>
#include <future>

#include "example/echo.pb.h"
#include "util/common.h"
#include "util/service.h"
#include "client/client_channel.h"
#include "scheduler/scheduler.h"

using namespace xuanqiong;

// ============ 压测配置 ============
constexpr int kConcurrency = 16;           // 并发线程数（建议设为 CPU 核数）
constexpr int kTotalRequests = 500000;    // 总请求数
constexpr bool kRecordLatency = true;     // 是否记录延迟（影响性能，可关闭）
constexpr const char* kServerAddr = "127.0.0.1";
constexpr int kServerPort = 8888;
// =================================

std::atomic<int> g_sent{0};
std::atomic<int> g_completed{0};

// 延迟统计（纳秒）
std::vector<std::chrono::nanoseconds> g_latencies;
std::mutex g_latencies_mutex;

// 回调函数：处理响应 + 记录延迟
void handle_response(
    EchoResponse* response,
    std::chrono::steady_clock::time_point start_time) {
    
    auto end = std::chrono::steady_clock::now();
    auto latency = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start_time);

    if (kRecordLatency) {
        std::lock_guard<std::mutex> lk(g_latencies_mutex);
        g_latencies.push_back(latency);
    }

    // 注意：压测时不要打印日志！否则严重拖慢性能
    // info("response: {}", response->DebugString());

    delete response;
    ++g_completed;
}

// 每个工作线程执行的函数
void worker_thread(std::shared_ptr<ClientChannel> channel, int worker_id) {
    EchoService_Stub stub(channel.get());  // 每个线程一个 stub（假设线程安全）

    while (true) {
        int idx = g_sent.fetch_add(1);
        if (idx >= kTotalRequests) break;

        auto request = new EchoRequest();
        request->set_message("echo_req_" + std::to_string(idx));

        auto response = new EchoResponse;
        auto controller = new RpcController();

        auto start_time = std::chrono::steady_clock::now();
        auto done = google::protobuf::NewCallback(&handle_response, response, start_time);

        stub.Echo(controller, request, response, done);
    }
}

int main() {
    std::cout << "Starting benchmark: "
              << kTotalRequests << " requests, "
              << kConcurrency << " threads\n";
    // 初始化调度器和执行器
    SchedulerOptions sched_options(1000, SchedPolicy::POLL_POLICY);
    Scheduler scheduler(sched_options);
    auto executor = scheduler.alloc_executor();

    // 启动工作线程
    std::vector<std::thread> workers;
    std::vector<std::shared_ptr<ClientChannel>> channels;
    auto test_start = std::chrono::steady_clock::now();

    for (int i = 0; i < kConcurrency; ++i) {
        auto ch = std::make_shared<ClientChannel>(kServerAddr, kServerPort, executor);
        channels.push_back(ch);
        workers.emplace_back(worker_thread, ch, i);
    }

    // 等待所有请求完成（或超时）
    constexpr int kMaxWaitSec = 60;
    while (g_completed < kTotalRequests) {
        auto elapsed = std::chrono::steady_clock::now() - test_start;
        if (elapsed > std::chrono::seconds(kMaxWaitSec)) {
            std::cerr << "Timeout! Only completed " << g_completed << " requests.\n";
            break;
        }
        std::cout << "Completed: " << g_completed << " / " << kTotalRequests << "\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    auto test_end = std::chrono::steady_clock::now();

    std::cout << "--------------\n";
    scheduler.stop();

    // 等待工作线程结束
    for (auto& t : workers) {
        if (t.joinable()) t.join();
    }

    // === 输出结果 ===
    double total_sec = std::chrono::duration<double>(test_end - test_start).count();
    double qps = static_cast<double>(g_completed) / total_sec;

    std::cout << "\n=== Benchmark Result ===\n";
    std::cout << "Completed: " << g_completed << " / " << kTotalRequests << "\n";
    std::cout << "Total time: " << total_sec << " s\n";
    std::cout << "QPS: " << static_cast<long long>(qps) << "\n";

    if (kRecordLatency && !g_latencies.empty()) {
        std::sort(g_latencies.begin(), g_latencies.end());
        auto size = g_latencies.size();

        auto p50 = g_latencies[size * 0.50];
        auto p90 = g_latencies[size * 0.90];
        auto p99 = g_latencies[size * 0.99];
        auto p999 = g_latencies[size * 0.999];

        std::cout << "Raw P50 (ns): " << g_latencies[size * 0.50].count() << "\n";

        auto to_us = [](auto ns) { return ns.count() / 1000.0; };

        std::cout << "Latency (us):\n";
        std::cout << "  P50:  " << to_us(p50) << "\n";
        std::cout << "  P90:  " << to_us(p90) << "\n";
        std::cout << "  P99:  " << to_us(p99) << "\n";
        std::cout << "  P99.9:" << to_us(p999) << "\n";
    }

    return 0;
}