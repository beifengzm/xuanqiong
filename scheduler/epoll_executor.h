#pragma once

#include <thread>
#include <memory>

#include "util/mpmc_queue.h"
#include "util/mpmc_queue.h"
#include "scheduler/scheduler.h"

namespace xuanqiong {

struct EventItem;
namespace net {
class Socket;
}

class EpollExecutor : public Executor {
public:
    // timeout: timeout in milliseconds
    EpollExecutor(int timeout);
    ~EpollExecutor();

    bool register_event(const EventItem& event_item) override;

    void stop() override;

    bool spawn(Closure&& task) override;

private:
    constexpr static int kTaskQueueCapacity = 4096;

    // task queue
    util::MPMCQueue<Closure> task_queue_;

    // for event notification
    std::atomic<bool> should_notify_{false};
    std::unique_ptr<net::Socket> dummy_socket_;

    // within a single thread, queue does not require lock
    std::unique_ptr<std::thread> thread_;
    int epoll_fd_;
    bool stop_;

    DISALLOW_COPY_AND_ASSIGN(EpollExecutor);
};

} // namespace xuanqiong
