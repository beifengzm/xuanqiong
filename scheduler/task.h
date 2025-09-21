#pragma once

#include <memory>
#include <coroutine>
#include <exception>

#include "net/socket.h"

namespace xuanqiong {

struct Task {
    struct promise_type;
    using HandleType = std::coroutine_handle<promise_type>;

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

struct Task::promise_type {
    Task get_return_object() {
        return Task(std::coroutine_handle<promise_type>::from_promise(*this));
    }
    std::suspend_always initial_suspend() { return {}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void unhandled_exception() {
        std::terminate();
    }
    void return_void() {}
};

} // namespace xuanqiong
