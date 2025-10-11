#ifdef __APPLE__

#include "scheduler/kqueue_executor.h"

#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "util/common.h"

#include <vector>

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
                error("kevent wait failed: %s", strerror(errno));
                continue;
            }

            for (int i = 0; i < nready; ++i) {
                void* udata = events[i].udata;
                if (udata == nullptr) {
                    continue;
                }
                auto handle = std::coroutine_handle<>::from_address(udata);
                handle.resume();
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
    struct kevent change = {};

    switch (event_item.type) {
        case EventType::READ: {
            EV_SET(&change,
                   static_cast<uintptr_t>(event_item.fd),
                   EVFILT_READ,
                   EV_ADD | EV_CLEAR,
                   0, 0,
                   event_item.handle.address());
            break;
        }

        case EventType::ADD_WRITE: {
            EV_SET(&change,
                   static_cast<uintptr_t>(event_item.fd),
                   EVFILT_WRITE,
                   EV_ADD | EV_CLEAR,
                   0, 0,
                   event_item.handle.address());
            break;
        }

        case EventType::DEL_WRITE: {
            EV_SET(&change,
                   static_cast<uintptr_t>(event_item.fd),
                   EVFILT_WRITE,
                   EV_DELETE,
                   0, 0,
                   nullptr);
            break;
        }

        case EventType::DELETE: {
            EV_SET(&change,
                   static_cast<uintptr_t>(event_item.fd),
                   EVFILT_READ,
                   EV_DELETE,
                   0, 0,
                   nullptr);
            if (kevent(kq_fd_, &change, 1, nullptr, 0, nullptr) == -1) {
            }

            EV_SET(&change,
                   static_cast<uintptr_t>(event_item.fd),
                   EVFILT_WRITE,
                   EV_DELETE,
                   0, 0,
                   nullptr);
            if (kevent(kq_fd_, &change, 1, nullptr, 0, nullptr) == -1) {
            }

            return true;
        }

        case EventType::UNKNOWN:
            error("EventType::UNKNOWN is invalid");
            return false;

        default:
            error("Unsupported event type: %d", static_cast<int>(event_item.type));
            return false;
    }

    if (kevent(kq_fd_, &change, 1, nullptr, 0, nullptr) == -1) {
        error("kevent register failed for fd=%d, type=%d: %s",
              event_item.fd, static_cast<int>(event_item.type), strerror(errno));
        return false;
    }

    return true;
}

} // namespace xuanqiong

#endif
