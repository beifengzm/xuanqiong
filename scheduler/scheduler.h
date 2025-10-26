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

    template<typename Func, typename... Args>
    void spawn(Func&& func, Args&&... args) {
        std::invoke(std::forward<Func>(func), std::forward<Args>(args)...);
    }
};

enum class SchedPolicy : uint8_t {
    POLL_POLICY,
    URING_POLICY,
};

// coroutine scheduler
class Scheduler {
public:
    Scheduler(SchedPolicy policy);
    ~Scheduler() = default;

    Executor* alloc_executor() { return executor_.get(); }

private:
    std::unique_ptr<Executor> executor_;

    DISALLOW_COPY_AND_ASSIGN(Scheduler);
};

} // namespace xuanqiong
