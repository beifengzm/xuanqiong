#pragma once

class EpollScheduler : public Scheduler {
public:
    EpollScheduler();
    ~EpollScheduler();

    void schedule() override {}
};
