#pragma once

#include <coroutine>

namespace xuanqiong {

enum struct EventType : uint8_t {
    READ,
    WRITE,
    OTHER,
};

struct SchedItem {
    int fd;
    EventType type;
    std::coroutine_handle<> handle;
};

template<class Executor>
concept IsExecutor = requires(Executor executor) {
    Executor();   // default constructible
    executor.schedule(std::declval<SchedItem>());
};

// coroutine scheduler
template<IsExecutor Executor>
class Scheduler {
public:
    Scheduler() : executor_(new Executor) {}
    ~Scheduler() {
        if (executor_) {
            delete executor_;
        }
    }

    Executor* alloc_executor() { return executor_; }

protected:
    Executor* executor_;
};

} // namespace xuanqiong
