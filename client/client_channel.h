#pragma once

#include <string>
#include <memory>

#include "net/socket.h"
#include "scheduler/task.h"
#include "util/output_stream.h"

namespace xuanqiong {

class Executor;

struct ClientPromise {
    int client_fd;
    Executor* executor_;

    ClientPromise(int client_fd, Executor* executor)
        : client_fd(client_fd), executor_(executor) {}

    Task<ClientPromise> get_return_object() {
        return Task<ClientPromise>(
            std::coroutine_handle<ClientPromise>::from_promise(*this)
        );
    }

    InitAwaiter initial_suspend() { return {client_fd, executor_}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void unhandled_exception() {
        std::terminate();
    }
    void return_void() {}
};

class ClientChannel {
public:
    ClientChannel(const std::string& ip, int port, Executor* executor);

    void close();

    util::NetOutputStream get_output_stream() {
        return socket_->get_output_stream();
    }

    template<typename Request, typename Response>
    void call_method(const Request* request,
                     Response* response,
                     const std::string& service_name,
                     const std::string& method_name);

private:
    std::unique_ptr<net::Socket> socket_;
    Executor* executor_;

    DISALLOW_COPY_AND_ASSIGN(ClientChannel);
};

} // namespace xuanqiong
