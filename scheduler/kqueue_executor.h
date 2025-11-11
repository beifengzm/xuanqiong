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
    bool spawn(Closure&& task) override;

private:
    static constexpr int kTaskQueueCapacity = 4096;

    util::MPMCQueue<Closure> task_queue_{kTaskQueueCapacity};
    std::atomic<bool> need_notify_{true};  // start as true to allow first notify
    std::unique_ptr<std::thread> thread_;
    int kq_fd_{-1};
    bool stop_{false};

    DISALLOW_COPY_AND_ASSIGN(KqueueExecutor);
};

} // namespace xuanqiong