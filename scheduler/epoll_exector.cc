#ifdef __linux__

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <string.h>

#include "util/common.h"
#include "net/socket.h"
#include "scheduler/scheduler.h"
#include "scheduler/epoll_executor.h"

#define MAX_EVENTS 1024

namespace xuanqiong {

EpollExecutor::EpollExecutor(int timeout) {
    epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd_ == -1) {
        error("epoll_create1 failed: %s", strerror(errno));
    }

    int event_fd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    info("event_fd: {}", event_fd);
    if (event_fd == -1) {
        error("eventfd failed: %s", strerror(errno));
    }
    dummy_socket_ = std::make_unique<net::Socket>(event_fd, nullptr, true);
    // add event_fd to epoll
    struct epoll_event ev;
    ev.data.ptr = dummy_socket_.get();
    ev.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, event_fd, &ev) == -1) {
        error("epoll_ctl failed: %s", strerror(errno));
    }

    thread_ = std::make_unique<std::thread>([this, timeout]() {
        while (!stop_) {
            Closure task;
            while (task_queue_.pop(task)) {
                task();
            }

            should_notify_.store(true, std::memory_order_release);
            struct epoll_event events[MAX_EVENTS];
            int nready = epoll_wait(epoll_fd_, events, MAX_EVENTS, timeout);
            if (nready == -1) {
                error("epoll_wait failed: %s", strerror(errno));
                continue;
            }
            // error("epoll_wait nready: {}", nready);
            for (int i = 0; i < nready; i++) {
                auto socket = reinterpret_cast<net::Socket*>(events[i].data.ptr);
                if (!socket) {
                    error("socket is null");
                    continue;
                }
                if (socket->is_dummy()) {
                    uint64_t val;
                    while (read(socket->fd(), &val, sizeof(uint64_t)) > 0);
                    // already awake, no need to notify
                    should_notify_.store(false, std::memory_order_release);
                    continue;
                }
                if (events[i].events & (EPOLLHUP | EPOLLRDHUP)) {
                    // handle error event
                    socket->close();
                    if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, socket->fd(), nullptr) == -1) {
                        error("epoll_ctl failed: {}", strerror(errno));
                    }
                    continue;
                }
                if (events[i].events & EPOLLOUT) {
                    // handle write event
                    struct epoll_event ev;
                    ev.data.ptr = socket;
                    ev.events = EPOLLIN | EPOLLET;
                    if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, socket->fd(), &ev) == -1) {
                        error("epoll_ctl failed: {}", strerror(errno));
                        continue;
                    }
                    socket->resume_write();
                }
                if (events[i].events & EPOLLIN) {
                    // handle read event
                    socket->resume_read();
                }
            }
        }
    });
}

EpollExecutor::~EpollExecutor() {
    if (thread_ && thread_->joinable()) {
        thread_->join();
    }
    if (epoll_fd_ != -1) {
        ::close(epoll_fd_);
        epoll_fd_ = -1;
    }
}

void EpollExecutor::stop() {
    stop_ = true;
}

bool EpollExecutor::spawn(Closure&& task) {
    if (!task_queue_.push(std::move(task))) {
        error("task queue is full");
        return false;
    }
    // notify epoll to wake up to execute task
    bool expect = true;
    if (should_notify_.compare_exchange_strong(expect, false, std::memory_order_release)) {
        uint64_t val = 1;
        write(dummy_socket_->fd(), &val, sizeof(uint64_t));
    }
    return true;
}

bool EpollExecutor::register_event(const EventItem& event_item) {
    struct epoll_event ev;
    ev.data.ptr = event_item.socket;
    switch (event_item.type) {
        case EventType::READ:
            ev.events = EPOLLIN | EPOLLET;  // edge triggered
            if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, event_item.socket->fd(), &ev) == -1) {
                error("epoll_ctl failed: {}", strerror(errno));
                return false;
            }
            break;
        case EventType::WRITE:
            ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
            if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, event_item.socket->fd(), &ev) == -1) {
                error("epoll_ctl failed: {}", strerror(errno));
                return false;
            }
            break;
        case EventType::DELETE:
            if (epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, event_item.socket->fd(), nullptr) == -1) {
                error("epoll_ctl failed: {}", strerror(errno));
                return false;
            }
            break;
        case EventType::UNKNOWN:
            ev.events = 0;
            break;
    default:
        error("unknown event type: {}", static_cast<int>(event_item.type));
        return false;
    }
    return true;
}

} // namespace xuanqiong

#endif
