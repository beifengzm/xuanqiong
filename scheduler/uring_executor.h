#pragma once

#include <thread>
#include <memory>
#include <liburing.h>

#include "util/mpmc_queue.h"
#include "scheduler/scheduler.h"

namespace xuanqiong {

struct EventItem;
namespace net {
class UringConnection;
}

class UringExecutor : public Executor {
public:
    // timeout: timeout in milliseconds
    UringExecutor(int timeout);
    ~UringExecutor();

    bool add_event(const EventItem& event_item) override {
        return true;
    }

    void stop() override;

    bool spawn(Closure&& task) override;

    io_uring* uring() { return &uring_; }

private:
    // task queue
    util::MPMCQueue<Closure> task_queue_;

    io_uring uring_;

    // for event notification
    std::atomic<bool> should_notify_{false};
    std::unique_ptr<net::UringConnection> dummy_conn_;

    // within a single thread, queue does not require lock
    std::unique_ptr<std::thread> thread_;
    int epoll_fd_;
    bool stop_;

    DISALLOW_COPY_AND_ASSIGN(UringExecutor);
};

} // namespace xuanqiong
