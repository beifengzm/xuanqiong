#pragma once

#include <thread>
#include <memory>

#include "util/mpmc_queue.h"
#include "scheduler/scheduler.h"

namespace xuanqiong {

class KqueueExecutor : public Executor {
public:
    KqueueExecutor(int timeout_ms);
    ~KqueueExecutor();

    bool register_event(const EventItem& event_item) override;

    void stop() override;

    bool spawn(Task&& task) override;

private:
    // task queue
    util::MPMCQueue<Task> task_queue_;

    // event notification fd
    std::atomic<bool> need_notify_{false};

    std::unique_ptr<std::thread> thread_;
    int kq_fd_;
    bool stop_;

    DISALLOW_COPY_AND_ASSIGN(KqueueExecutor);
};

} // namespace xuanqiong
