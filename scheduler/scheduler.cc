#include "scheduler/scheduler.h"
#ifdef __APPLE__
#include "scheduler/kqueue_executor.h"
#else
#include "scheduler/epoll_executor.h"
#endif

namespace xuanqiong {

Scheduler::Scheduler(const SchedulerOptions& options) {
    switch (options.policy) {
        case SchedPolicy::POLL_POLICY:
#ifdef __APPLE__
            executor_ = std::make_unique<KqueueExecutor>(options.poll_timeout);
#else
            executor_ = std::make_unique<EpollExecutor>(options.poll_timeout);
#endif
            break;
        case SchedPolicy::URING_POLICY:
            // executor_ = std::make_unique<UringExecutor>();
            break;
    }
}

} // namespace xuanqiong
