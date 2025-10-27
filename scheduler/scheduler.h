#pragma once

#include <coroutine>
#include <memory>
#include <functional>

#include "util/common.h"

namespace xuanqiong {

namespace net {
class Socket;
}

enum struct EventType : uint8_t {
    READ,
    WRITE,
    DELETE,     // remove fd from scheduler
    UNKNOWN,
};

struct EventItem {
    EventType type;
    net::Socket* socket;
};

// one Executor corresponds to one Thread, a group of Coroutines
class Executor {
public:
    Executor() = default;
    virtual ~Executor() = default;

    virtual bool register_event(const EventItem& item) = 0;

    virtual void stop() = 0;

    template<typename Func, typename... Args>
    void spawn(Func&& func, Args&&... args) {
        std::invoke(std::forward<Func>(func), std::forward<Args>(args)...);
    }
};

enum class SchedPolicy : uint8_t {
    POLL_POLICY,
    URING_POLICY,
};

struct SchedulerOptions {
    int poll_timeout;
    SchedPolicy policy;
    SchedulerOptions(int poll_timeout = -1, SchedPolicy policy = SchedPolicy::POLL_POLICY)
        : poll_timeout(poll_timeout), policy(policy) {}
};

// coroutine scheduler
class Scheduler {
public:
    Scheduler(const SchedulerOptions& options);
    ~Scheduler() = default;

    void stop() {
        executor_->stop();
    }

    Executor* alloc_executor() { return executor_.get(); }

private:
    std::unique_ptr<Executor> executor_;

    DISALLOW_COPY_AND_ASSIGN(Scheduler);
};

} // namespace xuanqiong
