#ifdef __APPLE__

#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <vector>

#include "util/common.h"
#include "net/socket.h"
#include "scheduler/kqueue_executor.h"

namespace xuanqiong {

static constexpr int MAX_EVENTS = 1024;

KqueueExecutor::KqueueExecutor(int timeout) : stop_(false) {
    kq_fd_ = kqueue();
    if (kq_fd_ == -1) {
        error("kqueue() failed: %s", strerror(errno));
        return;
    }

    // Register EVFILT_USER for task notification
    struct kevent user_event;
    EV_SET(&user_event, 1, EVFILT_USER, EV_ADD | EV_CLEAR, 0, 0, nullptr);
    if (kevent(kq_fd_, &user_event, 1, nullptr, 0, nullptr) == -1) {
        error("failed to add EVFILT_USER: %s", strerror(errno));
    }

    thread_ = std::make_unique<std::thread>([this, timeout]() {
        std::vector<struct kevent> events(MAX_EVENTS);
        while (!stop_) {
            // Process pending tasks first
            Closure task;
            while (task_queue_.pop(task)) {
                task();
            }

            struct timespec* timeout_ptr = nullptr;
            struct timespec timeout_spec;
            if (timeout >= 0) {
                timeout_spec.tv_sec = timeout / 1000;
                timeout_spec.tv_nsec = (timeout % 1000) * 1'000'000LL;
                timeout_ptr = &timeout_spec;
            }

            // Prepare to wait: only notify if queue might have new tasks
            should_notify_.store(true, std::memory_order_release);
            int nready = kevent(kq_fd_, nullptr, 0, events.data(), MAX_EVENTS, timeout_ptr);
            if (nready == -1) {
                if (errno == EINTR) continue;
                error("kevent wait failed: %s", strerror(errno));
                continue;
            }

            // info("kevent wait {} events", nready);
            for (int i = 0; i < nready; ++i) {
                auto& ev = events[i];

                if (ev.filter == EVFILT_USER) {
                    // Task notification: reset flag and continue loop to process tasks
                    should_notify_.store(false, std::memory_order_release);
                    continue;
                }

                auto socket = static_cast<net::Socket*>(ev.udata);
                if (!socket) {
                    error("socket is null in kevent");
                    continue;
                }

                if (ev.flags & EV_EOF) {
                    info("socket fd=%d closed (EV_EOF)", socket->fd());
                    socket->close();
                    // Clean up filters
                    struct kevent del_ev[2];
                    EV_SET(&del_ev[0], ev.ident, EVFILT_READ,  EV_DELETE, 0, 0, nullptr);
                    EV_SET(&del_ev[1], ev.ident, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
                    kevent(kq_fd_, del_ev, 2, nullptr, 0, nullptr);
                    continue;
                }

                if (ev.filter == EVFILT_WRITE) {
                    // One-shot: delete write event after handling
                    struct kevent del_ev;
                    EV_SET(&del_ev, ev.ident, EVFILT_WRITE, EV_DELETE, 0, 0, nullptr);
                    kevent(kq_fd_, &del_ev, 1, nullptr, 0, nullptr);
                    socket->resume_write();
                } else if (ev.filter == EVFILT_READ) {
                    socket->resume_read();
                }
            }
        }
    });
}

KqueueExecutor::~KqueueExecutor() {
    stop();
    if (thread_ && thread_->joinable()) {
        thread_->join();
    }
    if (kq_fd_ != -1) {
        ::close(kq_fd_);
        kq_fd_ = -1;
    }
}

void KqueueExecutor::stop() {
    stop_ = true;
    if (kq_fd_ != -1) {
        // Trigger user event to wake up the loop
        struct kevent trigger;
        EV_SET(&trigger, 1, EVFILT_USER, 0, NOTE_TRIGGER, 0, nullptr);
        kevent(kq_fd_, &trigger, 1, nullptr, 0, nullptr);
    }
}

bool KqueueExecutor::spawn(Closure&& task) {
    if (!task_queue_.push(std::move(task))) {
        error("failed to push task to queue");
        return false;
    }

    // Only notify if we haven't already signaled (to reduce syscalls)
    bool expected = true;
    if (should_notify_.compare_exchange_strong(expected, false, std::memory_order_acq_rel)) {
        struct kevent trigger;
        EV_SET(&trigger, 1, EVFILT_USER, 0, NOTE_TRIGGER, 0, nullptr);
        if (kevent(kq_fd_, &trigger, 1, nullptr, 0, nullptr) == -1) {
            error("failed to trigger EVFILT_USER: %s", strerror(errno));
            // Re-arm notify flag on failure so next spawn can try again
            should_notify_.store(true, std::memory_order_release);
            return false;
        }
    }
    return true;
}

bool KqueueExecutor::register_event(const EventItem& event_item) {
    int fd = event_item.socket->fd();
    void* udata = event_item.socket;

    switch (event_item.type) {
        case EventType::READ: {
            struct kevent ev;
            EV_SET(&ev, static_cast<uintptr_t>(fd), EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, udata);
            if (kevent(kq_fd_, &ev, 1, nullptr, 0, nullptr) == -1) {
                error("kqueue add READ failed for fd=%d: %s", fd, strerror(errno));
                return false;
            }
            break;
        }
        case EventType::WRITE: {
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
            kevent(kq_fd_, del_ev, 2, nullptr, 0, nullptr);
            break;
        }
        default:
            error("Unsupported or invalid event type: %d", static_cast<int>(event_item.type));
            return false;
    }
    return true;
}

} // namespace xuanqiong

#endif