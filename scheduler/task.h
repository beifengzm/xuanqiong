#pragma once

#include <coroutine>
#include <exception>

#include "coro/socket.h"

namespace xuanqiong {

struct Task {
    struct promise_type {
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

    using HandleType = std::coroutine_handle<promise_type>;

    Task(HandleType handle) : handle_(handle) {}
    ~Task() {
        if (handle_) {
            handle_.destroy();
        }
    }

private:
    HandleType handle_;
};

Task handle_request(Socket& socket) {
    co_await socket.read(request);
    return {};
}

} // namespace xuanqiong
