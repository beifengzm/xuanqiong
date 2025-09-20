#pragma once

class Scheduler {
public:
    Scheduler();
    virtual ~Scheduler();

    virtual void schedule() = 0;
};
