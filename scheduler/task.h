#pragma once

#include <coroutine>

namespace xuanqiong {

struct Task {
    struct promise_type {
        Task get_return_object() {
            return Task(std::coroutine_handle<promise_type>::from_promise(*this));
        }
        std::suspend_never initial_suspend() { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void unhandled_exception() {
            std::terminate();
        }
        void return_void() {}
    };

    using HandleType = std::coroutine_handle<promise_type>;

    Task(HandleType handle) : handle_(handle) {}

private:
    HandleType handle_;

    DISALLOW_COPY_AND_ASSIGN(Task);
};

} // namespace xuanqiong
