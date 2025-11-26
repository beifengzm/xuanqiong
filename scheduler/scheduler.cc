#include "scheduler/scheduler.h"
#ifdef __APPLE__
#include "scheduler/kqueue_executor.h"
#else
#include "scheduler/epoll_executor.h"
#include "scheduler/uring_executor.h"
#endif

namespace xuanqiong {

Scheduler::Scheduler(const SchedulerOptions& options) {
    switch (options.policy) {
        case SchedPolicy::POLL_POLICY:
#ifdef __APPLE__
        case SchedPolicy::URING_POLICY:
            executor_ = std::make_unique<KqueueExecutor>(options.timeout);
            break;
#else
            executor_ = std::make_unique<EpollExecutor>(options.timeout);
            break;
        case SchedPolicy::URING_POLICY:
            executor_ = std::make_unique<UringExecutor>(options.timeout);
            break;
#endif
    }
}

} // namespace xuanqiong
