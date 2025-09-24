#pragma once

#include <memory>
#include <coroutine>
#include <exception>

#include "net/channel.h"
#include "scheduler/task.h"

namespace xuanqiong {

class Executor;

struct ServerTask {
    ServerTask(int client_fd, Executor* executor)
        : channel_(std::make_unique<net::Channel>(client_fd)), executor_(executor) {}

    void run() {
        while (true) {
            int nread = co_await channel_->read_data();
            if (nread == -1) {
                error("error occurred in read: %s", strerror(errno));
                continue;
            }
        }
    }

    // void handle_request() {
    //     while (true) {
    //         int nread = co_await channel_->read_data();
    //         if (nread == -1) {
    //             error("error occurred in read: %s", strerror(errno));
    //             continue;
    //         }
    //     }
    // }

private:
    std::unique_ptr<net::Channel> channel_;
    Executor* executor_;
};

} // namespace xuanqiong
