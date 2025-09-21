#include <sys/epoll.h>

#include "scheduler/task.h"
#include "scheduler/epoll_scheduler.h"

constexpr static int MAX_EVENTS = 1024;

WorkerGroup::WorkerGroup() {
    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ == -1) {
        error("epoll_create1 failed: %s", strerror(errno));
    }

    thread_ = std::thread([this]() {
        while (true) {
            struct epoll_event events[MAX_EVENTS];
            int ready_num = epoll_wait(epoll_fd_, events, MAX_EVENTS, -1);
            if (ready_num == -1) {
                error("epoll_wait failed: %s", strerror(errno));
                continue;
            }
            for (int i = 0; i < ready_num; i++) {
                auto task = static_cast<Task*>(events[i].data.ptr);
                task.resume();
            }
        }
    });
}

WorkerGroup::~WorkerGroup() {
    if (thread_.joinable()) {
        thread_.join();
    }
}

template<typename Task>
WorkerGroup<Task>::~WorkerGroup() {
    if (epoll_fd_ != -1) {
        close(epoll_fd_);
        epoll_fd_ = -1;
    }
}
