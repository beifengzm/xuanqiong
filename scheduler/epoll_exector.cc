#ifdef __linux__

#include <sys/epoll.h>
#include <string.h>

#include "util/common.h"
#include "net/socket.h"
#include "scheduler/scheduler.h"
#include "scheduler/epoll_executor.h"

#define MAX_EVENTS 1024

namespace xuanqiong {

EpollExecutor::EpollExecutor() {
    epoll_fd_ = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd_ == -1) {
        error("epoll_create1 failed: %s", strerror(errno));
    }
    thread_ = std::make_unique<std::thread>([this]() {
        while (!stop_) {
            struct epoll_event events[MAX_EVENTS];
            int nready = epoll_wait(epoll_fd_, events, MAX_EVENTS, -1);
            if (nready == -1) {
                error("epoll_wait failed: %s", strerror(errno));
                continue;
            }
            error("epoll_wait nready: {}", nready);
            for (int i = 0; i < nready; i++) {
                // TODO: handle error event
                auto sock =
                    reinterpret_cast<net::Socket*>(events[i].data.ptr);
                sock->resume();
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
            ev.events = EPOLLOUT | EPOLLET;
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
