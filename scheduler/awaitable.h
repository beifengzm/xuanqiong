#pragma once

#include <coroutine>

#include "scheduler/scheduler.h"

namespace xuanqiong {

namespace net {
class Socket;
}

// when the coroutine function starts executing,
// it begins waiting for the socket read event
struct InitAwaiter {
    std::shared_ptr<net::Socket> socket;

    bool await_ready() const noexcept { return false; }
    bool await_suspend(std::coroutine_handle<> handle) noexcept;
    bool await_resume() const noexcept { return false; }
};

struct ReadAwaiter {
    net::Socket* socket;

    bool await_ready() const noexcept { return false; }
    bool await_suspend(std::coroutine_handle<> handle) noexcept;
    bool await_resume() const noexcept;
};

struct WriteAwaiter {
    net::Socket* socket;
    // >0: more data to write, ==0: write done, <0: error
    int write_remain;

    bool await_ready() const noexcept { return write_remain <= 0; }
    void await_suspend(std::coroutine_handle<> handle) noexcept;
    int await_resume() const noexcept {
        return write_remain;
    }
};

} // namespace xuanqiong
