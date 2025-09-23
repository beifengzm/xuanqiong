#include <sys/epoll.h>

#include "scheduler/scheduler.h"
#include "scheduler/epoll_scheduler.h"

Executor::Executor() {
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
                auto handle =
                    std::coroutine_handle<>::from_address(events[i].data.ptr);
                handle.resume();
            }
        }
    });
}

Executor::~Executor() {
    if (epoll_fd_ != -1) {
        ::close(epoll_fd_);
        epoll_fd_ = -1;
    }
    if (thread_ && thread_->joinable()) {
        thread_->join();
    }
}

bool Executor::schedule(const SchedItem& sched_item) {
    struct epoll_event ev;
    ev.events = sched_item.type == EventType::READ ? EPOLLIN : EPOLLOUT;
    ev.data.ptr = sched_item.handle.address();
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, sched_item.fd, &ev) == -1) {
        error("epoll_ctl failed: %s", strerror(errno));
        return false;
    }
    return true;
}

