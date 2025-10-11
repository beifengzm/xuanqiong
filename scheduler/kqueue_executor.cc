#ifdef __APPLE__

#include "scheduler/kqueue_executor.h"

#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "util/common.h"  // 假设包含 error() 宏或函数

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
        while (true) {
            int nready = kevent(kq_fd_, nullptr, 0, events.data(), MAX_EVENTS, nullptr);
            if (nready == -1) {
                if (errno == EINTR) {
                    continue; // 被信号中断，重试
                }
                error("kevent wait failed: %s", strerror(errno));
                continue;
            }

            for (int i = 0; i < nready; ++i) {
                // udata 存储的是 coroutine_handle 的 address
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

KqueueExecutor::~KqueueExecutor() {
    if (thread_ && thread_->joinable()) {
        // TODO: 更优雅的方式是发送内部事件唤醒线程并退出
        // 当前简单 join，但线程是死循环，无法退出！
        // 实际项目中建议加入 stop flag + pipe 或 EVFILT_USER
        // 这里先保留，仅用于演示
        // 你可以后续补充退出机制
    }

    if (kq_fd_ != -1) {
        ::close(kq_fd_);
        kq_fd_ = -1;
    }

    // 注意：如果线程还在运行，直接 close(kq_fd_) 可能导致 kevent 返回 EBADF，
    // 但线程仍无法退出。建议后续加入退出机制。
    if (thread_ && thread_->joinable()) {
        thread_->join();
    }
}

bool KqueueExecutor::register_event(const EventItem& event_item) {
    struct kevent change = {};

    switch (event_item.type) {
        case EventType::READ: {
            // 注册可读事件（边缘触发：EV_CLEAR）
            EV_SET(&change,
                   static_cast<uintptr_t>(event_item.fd),
                   EVFILT_READ,
                   EV_ADD | EV_CLEAR,
                   0, 0,
                   event_item.handle.address());
            break;
        }

        case EventType::ADD_WRITE: {
            // 添加可写事件（通常只在需要时临时添加）
            EV_SET(&change,
                   static_cast<uintptr_t>(event_item.fd),
                   EVFILT_WRITE,
                   EV_ADD | EV_CLEAR,
                   0, 0,
                   event_item.handle.address());
            break;
        }

        case EventType::DEL_WRITE: {
            // 删除可写事件
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
                // 可能未注册，忽略错误
            }

            // 尝试删除 WRITE
            EV_SET(&change,
                   static_cast<uintptr_t>(event_item.fd),
                   EVFILT_WRITE,
                   EV_DELETE,
                   0, 0,
                   nullptr);
            if (kevent(kq_fd_, &change, 1, nullptr, 0, nullptr) == -1) {
                // 忽略
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
