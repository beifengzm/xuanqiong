#include "awaitable.h"
#include "net/socket.h"

namespace xuanqiong {

// bool InitAwaiter::await_suspend(std::coroutine_handle<> handle) noexcept {
//     auto executor = socket->executor();
//     return executor->register_event({socket->fd(), EventType::READ, handle});
// }

bool ReadAwaiter::await_suspend(std::coroutine_handle<> handle) noexcept {
    if (socket->closed()) {
        // read EOF, connection close, destory coroutine
        auto executor = socket->executor();
        socket->set_coro_handle(handle.address());
        executor->register_event({EventType::DELETE, socket});
        return false;
    }
    return true;
}

void WriteAwaiter::await_suspend(std::coroutine_handle<> handle) noexcept {
    socket->set_coro_handle(handle.address());
    auto executor = socket->executor();
    executor->register_event({EventType::WRITE, socket});
}

} // namespace xuanqiong
