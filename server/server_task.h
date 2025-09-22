#pragma once

#include <memory>
#include <coroutine>
#include <exception>

#include "net/socket.h"
#include "scheduler/task.h"

namespace xuanqiong {

class Scheduler;

struct ServerTask {
    ServerTask(int client_fd, Scheduler* scheduler)
        : socket_(std::make_unique<net::Socket>(client_fd)), scheduler_(scheduler) {}

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
    Scheduler* scheduler_;
};

} // namespace xuanqiong
