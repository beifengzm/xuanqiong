#pragma once

namespace xuanqiong {

// coroutine scheduler
class Scheduler {
public:
    virtual ~Scheduler();

    virtual void schedule() = 0;
};

} // namespace xuanqiong
