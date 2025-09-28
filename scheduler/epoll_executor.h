#pragma once

#include <thread>
#include <memory>

#include "scheduler/scheduler.h"

namespace xuanqiong {

struct EventItem;

// one Executor corresponds to one Thread, a group of Coroutines
class EpollExecutor : public Executor {
public:
    EpollExecutor();
    ~EpollExecutor();

    bool register_event(const EventItem& event_item) override;

private:
    // within a single thread, queue does not require lock
    std::unique_ptr<std::thread> thread_;
    int epoll_fd_;
};

} // namespace xuanqiong
