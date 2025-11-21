#include "awaitable.h"
#include "net/connection.h"

namespace xuanqiong {

bool RegisterReadAwaiter::await_suspend(std::coroutine_handle<> handle) noexcept {
    conn->set_read_handle(handle.address());
    auto executor = conn->executor();
    executor->add_event({EventType::READ, conn});
    return false;
}

bool ReadAwaiter::await_suspend(std::coroutine_handle<> handle) noexcept {
    if (conn->closed()) {
        // read EOF, connection close, destory coroutine
        auto executor = conn->executor();
        executor->add_event({EventType::DELETE, conn});
    }
    return !conn->closed() && should_suspend;
}

void WriteAwaiter::await_suspend(std::coroutine_handle<> handle) noexcept {
    conn->set_write_handle(handle.address());
    auto executor = conn->executor();
    executor->add_event({EventType::WRITE, conn});
}

bool WaitWriteAwaiter::await_ready() const noexcept {
    return conn->closed() || conn->write_bytes() > 0;
}

void WaitWriteAwaiter::await_suspend(std::coroutine_handle<> handle) noexcept {
    conn->set_write_handle(handle.address());
}

} // namespace xuanqiong
