#ifdef __APPLE__

#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <utility>
#include <vector>

#include "util/common.h"
#include "net/socket.h"
#include "scheduler/kqueue_executor.h"

namespace xuanqiong {

static constexpr int MAX_EVENTS = 1024;

KqueueExecutor::KqueueExecutor() {
    kq_fd_ = kqueue();
    if (kq_fd_ == -1) {
        error("kqueue() failed: %s", strerror(errno));
        return;
    }

    thread_ = std::make_unique<std::thread>([this]() {
        std::vector<struct kevent> events(MAX_EVENTS);
        while (!stop_) {
            int nready = kevent(kq_fd_, nullptr, 0, events.data(), MAX_EVENTS, nullptr);
            if (nready == -1) {
                if (errno == EINTR) continue; // 可选：处理信号中断
                error("kevent wait failed: %s", strerror(errno));
                continue;
            }
            info("kevent wait {} events", nready);
            for (int i = 0; i < nready; ++i) {
                auto& ev = events[i];
                auto socket = static_cast<net::Socket*>(ev.udata);
                if (socket == nullptr) {
                    error("socket is null");
                    continue;
                }

                if (ev.flags & EV_EOF) {
                    info("socket fd=%d closed (EV_EOF)", socket->fd());
                    socket->close();
                    // 删除该 fd 的所有事件（READ 和 WRITE）
                    struct kevent del_ev[2];
                    EV_SET(&del_ev[0], ev.ident, EVFILT_READ,  EV_DELETE, 0, 0, nullptr);
                    EV_SET(&del_ev[1], ev.ident, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
                    kevent(kq_fd_, del_ev, 2, nullptr, 0, nullptr);
                    continue;
                }
                if (ev.filter == EVFILT_WRITE) {
                    struct kevent mod_ev;
                    EV_SET(&mod_ev, ev.ident, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
                    if (kevent(kq_fd_, &mod_ev, 1, nullptr, 0, nullptr) == -1) {
                        error("failed to delete EVFILT_WRITE for fd=%d: %s", (int)ev.ident, strerror(errno));
                    }
                    socket->resume_write();
                } else if (ev.filter == EVFILT_READ) {
                    socket->resume_read();
                }
            }
        }
    });
}

void KqueueExecutor::stop() {
    stop_ = true;
}

KqueueExecutor::~KqueueExecutor() {
    if (thread_ && thread_->joinable()) {
        thread_->join();
    }
    if (kq_fd_ != -1) {
        ::close(kq_fd_);
        kq_fd_ = -1;
    }
}

bool KqueueExecutor::register_event(const EventItem& event_item) {
    int fd = event_item.socket->fd();
    void* udata = static_cast<void*>(event_item.socket);

    switch (event_item.type) {
        case EventType::READ: {
            // 添加可读事件（边缘触发）
            struct kevent ev;
            EV_SET(&ev, static_cast<uintptr_t>(fd), EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, udata);
            if (kevent(kq_fd_, &ev, 1, nullptr, 0, nullptr) == -1) {
                error("kqueue add READ failed for fd=%d: %s", fd, strerror(errno));
                return false;
            }
            break;
        }
        case EventType::WRITE: {
            // 添加可写事件（边缘触发）
            // 注意：这里不删除 READ，因为通常读写是共存的，但写事件触发后我们会主动删 WRITE
            struct kevent ev;
            EV_SET(&ev, static_cast<uintptr_t>(fd), EVFILT_WRITE, EV_ADD | EV_CLEAR, 0, 0, udata);
            if (kevent(kq_fd_, &ev, 1, nullptr, 0, nullptr) == -1) {
                error("kqueue add WRITE failed for fd=%d: %s", fd, strerror(errno));
                return false;
            }
            break;
        }
        case EventType::DELETE: {
            info("delete event for fd=%d", fd);
            struct kevent del_ev[2];
            EV_SET(&del_ev[0], static_cast<uintptr_t>(fd), EVFILT_READ,  EV_DELETE, 0, 0, nullptr);
            EV_SET(&del_ev[1], static_cast<uintptr_t>(fd), EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
            // 忽略返回值，即使失败也不影响（fd 可能已关闭）
            kevent(kq_fd_, del_ev, 2, nullptr, 0, nullptr);
            break;
        }
        case EventType::UNKNOWN:
            error("EventType::UNKNOWN is invalid");
            return false;
        default:
            error("Unsupported event type: %d", static_cast<int>(event_item.type));
            return false;
    }
    return true;
}

} // namespace xuanqiong

#endif