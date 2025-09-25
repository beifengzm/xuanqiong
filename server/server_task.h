#pragma once

#include <memory>
#include <coroutine>
#include <exception>

#include "net/socket.h"
#include "scheduler/scheduler.h"
#include "scheduler/task.h"

namespace xuanqiong {

class Executor;

// suspend when coroutine is created
struct ServerInitAwaitable {
    int fd;
    Executor* executor_;

    ServerInitAwaitable(int fd, Executor* executor) : fd(fd), executor_(executor) {}

    bool await_ready() { return false; }
    void await_suspend(std::coroutine_handle<> handle) {
        executor_->schedule({fd, handle, EventType::READ});
    }
    void await_resume() {}
};

struct ServerPromise {
    int client_fd;
    Executor* executor_;

    ServerPromise(int client_fd, Executor* executor)
        : client_fd(client_fd), executor_(executor) {}

    Task get_return_object() {
        return Task(std::coroutine_handle<ServerPromise>::from_promise(*this));
    }
    ServerInitAwaitable initial_suspend() { return {client_fd, executor_}; }
    std::suspend_always final_suspend() noexcept { return {}; }
    void unhandled_exception() {
        std::terminate();
    }
    void return_void() {}
};

struct ServerTask {
    ServerTask(int client_fd, Executor* executor)
        : socket_(std::make_unique<net::Socket>(client_fd)), executor_(executor) {}

    void run();

    // void handle_request() {
    //     while (true) {
    //         int nread = co_await socket_->read_data();
    //         if (nread == -1) {
    //             error("error occurred in read: %s", strerror(errno));
    //             continue;
    //         }
    //     }
    // }

private:
    std::unique_ptr<net::Socket> socket_;
    
};

} // namespace xuanqiong
