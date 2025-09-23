#pragma once

#include <thread>
#include <memory>

namespace xuanqiong {

struct SchedItem;

// one Executor corresponds to one Thread, a group of Coroutines
class EpollExecutor : public Executor {
public:
    EpollExecutor();
    ~EpollExecutor();

    bool schedule(const SchedItem& sched_item) override;

private:
    // within a single thread, queue does not require lock
    std::unique_ptr<std::thread> thread_;
    int epoll_fd_;
};

} // namespace xuanqiong
