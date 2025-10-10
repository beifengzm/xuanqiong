#pragma once

#include <coroutine>
#include <memory>
#include <functional>

#include "util/common.h"

namespace xuanqiong {

enum struct EventType : uint8_t {
    READ,
    DELETE,     // remove fd from scheduler
    ADD_WRITE,
    DEL_WRITE,
    UNKNOWN,
};

struct EventItem {
    int fd;
    EventType type;
    std::coroutine_handle<> handle;
};

// one Executor corresponds to one Thread, a group of Coroutines
class Executor {
public:
    Executor() = default;
    ~Executor() = default;

    virtual bool register_event(const EventItem& item) = 0;

    template<typename Func, typename... Args>
    void spawn(Func&& func, Args&&... args) {
        std::invoke(std::forward<Func>(func), std::forward<Args>(args)...);
    }
};

enum class SchedPolicy : uint8_t {
    EPOLL_POLICY,
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
