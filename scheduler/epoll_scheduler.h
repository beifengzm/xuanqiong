#pragma once

#include <thread>
#include <memory>

namespace xuanqiong {

// one WorkerGroup corresponds to one Thread, a group of Coroutines
class WorkerGroup {
public:
    WorkerGroup();
    ~WorkerGroup();

private:
    // within a single thread, queue does not require lock
    std::unique_ptr<std::thread> thread_;
    int epoll_fd_;
};

class EpollScheduler : public Scheduler {
public:
    EpollScheduler();
    ~EpollScheduler();

    void schedule() override {}
};

} // namespace xuanqiong
