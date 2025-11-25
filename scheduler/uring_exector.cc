#ifdef __linux__

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <string.h>

#include "util/common.h"
#include "net/socket.h"
#include "scheduler/scheduler.h"
#include "net/uring_connection.h"
#include "scheduler/uring_executor.h"

#define MAX_RING_LEN 16384

namespace xuanqiong {

UringExecutor::UringExecutor(int timeout) {
    io_uring_queue_init(MAX_RING_LEN, &uring_, 0);

    int event_fd = eventfd(0, EFD_CLOEXEC);
    info("event_fd: {}", event_fd);
    if (event_fd == -1) {
        error("eventfd failed: %s", strerror(errno));
    }

    dummy_conn_ = std::make_unique<net::UringConnection>(event_fd, nullptr, true);

    thread_ = std::make_unique<std::thread>([this, timeout]() {
        while (!stop_) {
            Closure task;
            while (task_queue_.pop(task)) {
                task();
            }

            io_uring_submit(&uring_);

            should_notify_.store(true, std::memory_order_release);
            io_uring_cqe* cqe;
            if (io_uring_cq_ready(&uring_) == 0) {
                io_uring_wait_cqe(&uring_, &cqe);
            }

            while (io_uring_peek_cqe(&uring_, &cqe) == 0) {
                io_uring_cqe_seen(&uring_, cqe);
                auto event_type = static_cast<EventType>(cqe->user_data >> 56);
                auto conn_part = cqe->user_data & ((1ULL << 56) - 1);
                auto conn = reinterpret_cast<net::UringConnection*>(conn_part);
                if (!conn) {
                    error("conn is null");
                    continue;
                }

                if (conn->is_dummy()) {
                    // already awake, no need to notify
                    should_notify_.store(false, std::memory_order_release);
                    continue;
                }
                
                if (event_type == EventType::READ) {
                    if (cqe->res == 0) {
                        conn->close();
                    } else {
                        conn->recv_add(cqe->res);
                    }
                    conn->resume_read();
                }
                // else if (event_type == EventType::WRITE) {
                //     info("write fd={}, res={}", conn->fd(), cqe->res);
                //     conn->write_add(cqe->res);
                // }
            }
        }
        
    });
}

UringExecutor::~UringExecutor() {
    if (thread_ && thread_->joinable()) {
        thread_->join();
    }
    io_uring_queue_exit(&uring_);
    if (epoll_fd_ != -1) {
        ::close(epoll_fd_);
        epoll_fd_ = -1;
    }
}

void UringExecutor::stop() {
    stop_ = true;
}

bool UringExecutor::spawn(Closure&& task) {
    if (!task_queue_.push(std::move(task))) {
        error("task queue is full");
        return false;
    }
    // notify epoll to wake up to execute task
    bool expect = true;
    if (should_notify_.compare_exchange_strong(expect, false, std::memory_order_release)) {
        auto sqe = io_uring_get_sqe(&uring_);
        if (!sqe) {
            io_uring_submit(&uring_);
            sqe = io_uring_get_sqe(&uring_);
        }
        uint64_t val = 1;
        io_uring_prep_write(sqe, dummy_conn_->fd(), &val, sizeof(uint64_t), 0);
        sqe->user_data = (static_cast<uint64_t>(EventType::WRITE) << 56)
                | reinterpret_cast<uint64_t>(dummy_conn_.get());
        io_uring_submit(&uring_);
    }
    return true;
}

} // namespace xuanqiong

#endif
