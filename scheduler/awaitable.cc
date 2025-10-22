#include "awaitable.h"
#include "net/socket.h"

namespace xuanqiong {

bool InitAwaiter::await_suspend(std::coroutine_handle<> handle) noexcept {
    auto executor = socket->executor();
    return executor->register_event({socket->fd(), EventType::READ, handle});
}

bool ReadAwaiter::await_suspend(std::coroutine_handle<> handle) noexcept {
    if (socket->closed()) {
        // read EOF, connection close, destory coroutine
        auto executor = socket->executor();
        executor->register_event({socket->fd(), EventType::DELETE, handle});
        return false;
    }
    return true;
}

bool ReadAwaiter::await_resume() const noexcept { return socket->closed(); }

void WriteAwaiter::await_suspend(std::coroutine_handle<> handle) noexcept {
    auto executor = socket->executor();
    executor->register_event({socket->fd(), EventType::WRITE, handle});
}

} // namespace xuanqiong
