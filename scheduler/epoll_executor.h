#pragma once

#include <thread>
#include <memory>

#include "scheduler/scheduler.h"

namespace xuanqiong {

struct EventItem;

class EpollExecutor : public Executor {
public:
    EpollExecutor(int timeout_ms);
    ~EpollExecutor();

    bool register_event(const EventItem& event_item) override;

    void stop() override;

private:
    // within a single thread, queue does not require lock
    std::unique_ptr<std::thread> thread_;
    int epoll_fd_;
    bool stop_;

    DISALLOW_COPY_AND_ASSIGN(EpollExecutor);
};

} // namespace xuanqiong
