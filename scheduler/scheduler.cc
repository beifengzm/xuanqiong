#include "scheduler/scheduler.h"
#include "scheduler/epoll_executor.h"

namespace xuanqiong {

Scheduler::Scheduler(SchedPolicy policy) {
    switch (policy) {
        case SchedPolicy::EPOLL_POLICY:
            executor_ = std::make_unique<EpollExecutor>();
            break;
        case SchedPolicy::URING_POLICY:
            // executor_ = std::make_unique<UringExecutor>();
            break;
    }
}

} // namespace xuanqiong
