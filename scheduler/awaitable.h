#pragma once

#include <coroutine>

#include "scheduler/scheduler.h"

namespace xuanqiong {

// when the coroutine function starts executing,
// it begins waiting for the socket read event
struct InitAwaiter {
    int fd;
    Executor* executor;

    bool await_ready() const noexcept { return false; }
    bool await_suspend(std::coroutine_handle<> handle) noexcept {
        return executor->register_event({fd, EventType::READ, handle});
    }
    bool await_resume() const noexcept { return false; }
};

struct ReadAwaiter {
    int fd;
    bool closed;
    Executor* executor;

    bool await_ready() const noexcept { return false; }
    bool await_suspend(std::coroutine_handle<> handle) noexcept {
        if (closed) {
            // read EOF, connection close, destory coroutine
            executor->register_event({fd, EventType::DELETE, handle});
            return false;
        }
        return true;
    }
    bool await_resume() const noexcept { return closed; }
};

struct WriteAwaiter {
    int fd;
    // >0: more data to write, ==0: write done, <0: error
    int write_remain;
    Executor* executor;

    bool await_ready() const noexcept { return write_remain <= 0; }
    void await_suspend(std::coroutine_handle<> handle) noexcept {
        executor->register_event({fd, EventType::WRITE, handle});
    }
    int await_resume() const noexcept {
        return write_remain;
    }
};

} // namespace xuanqiong
