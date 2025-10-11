#pragma once

#include <thread>
#include <memory>

#include "scheduler/scheduler.h"

namespace xuanqiong {

class KqueueExecutor : public Executor {
public:
    KqueueExecutor();
    ~KqueueExecutor();

    bool register_event(const EventItem& event_item) override;

private:
    std::unique_ptr<std::thread> thread_;
    int kq_fd_;

    DISALLOW_COPY_AND_ASSIGN(KqueueExecutor);
};

} // namespace xuanqiong
