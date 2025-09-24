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

class Executor {
public:
    Executor() = default;
    ~Executor() = default;

    virtual void schedule(SchedItem item) = 0;
};

using ExecutorPtr = std::unique_ptr<Executor>;
using ExecutorBuilder = std::function<ExecutorPtr()>;

// coroutine scheduler
class Scheduler {
public:
    Scheduler(ExecutorBuilder builder) : executor_(builder()) {}
    ~Scheduler() = default;

    Executor* alloc_executor() { return executor_.get(); }

protected:
    ExecutorPtr executor_;
};

} // namespace xuanqiong
