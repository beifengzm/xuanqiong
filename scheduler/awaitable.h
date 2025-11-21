#pragma once

#include <coroutine>

#include "scheduler/scheduler.h"

namespace xuanqiong {

namespace net {
class Connection;
}

struct RegisterReadAwaiter {
    net::Connection* conn;

    bool await_ready() const noexcept { return false; }
    bool await_suspend(std::coroutine_handle<> handle) noexcept;
    void await_resume() const noexcept {};
};

struct ReadAwaiter {
    net::Connection* conn;
    bool should_suspend;

    bool await_ready() const noexcept { return false; }
    bool await_suspend(std::coroutine_handle<> handle) noexcept;
    void await_resume() const noexcept {};
};

struct WriteAwaiter {
    net::Connection* conn;
    // >0: more data to write, ==0: write done, <0: error
    bool should_suspend;

    bool await_ready() const noexcept { return !should_suspend; }
    void await_suspend(std::coroutine_handle<> handle) noexcept;
    void await_resume() const noexcept {}
};

// wait for write data ready
struct WaitWriteAwaiter {
    net::Connection* conn;

    bool await_ready() const noexcept;
    void await_suspend(std::coroutine_handle<> handle) noexcept;
    void await_resume() const noexcept {};
};

} // namespace xuanqiong
