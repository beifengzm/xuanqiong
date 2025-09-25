#pragma once

#include <memory>
#include <coroutine>
#include <exception>

#include "net/socket.h"

namespace xuanqiong {

template<typename Promise>
struct Task {
    using HandleType = std::coroutine_handle<Promise>;

    Task(HandleType handle) : handle_(handle) {}
    ~Task();

    void resume() {
        if (handle_) {
            handle_.resume();
        }
    }

private:
    HandleType handle_;
};

struct ReadAwaiter {
    int fd;
    int nread;           // total bytes read already
    Executor* executor;

    bool await_ready() { return closed; }
    bool await_suspend(std::coroutine_handle<> handle) {
        executor->schedule({fd, handle, EventType::READ});
    }
    bool await_resume() { return closed; }
};

} // namespace xuanqiong
