#pragma once

#include <coroutine>
#include <unistd.h>
#include <sys/socket.h>

#include "util/common.h"
#include "util/input_stream.h"
#include "util/output_stream.h"
#include "scheduler/awaitable.h"

namespace xuanqiong {
class Executor;
}

namespace xuanqiong::net {

class Socket {
public:
    Socket(int fd, Executor* executor);
    ~Socket();

    int fd() const { return sockfd_; }
    const std::string& local_addr() const { return local_addr_; }
    int local_port() const { return local_port_; }
    const std::string& peer_addr() const { return peer_addr_; }
    int peer_port() const { return peer_port_; }

    Executor* executor() const { return executor_; }

    // set and resume read/write coroutine handle
    void set_read_handle(void* handle) { read_handle_ = handle; }
    void set_write_handle(void* handle) { write_handle_ = handle; }
    void resume_read() const {
        std::coroutine_handle<>::from_address(read_handle_).resume();
    }
    void resume_write() const {
        std::coroutine_handle<>::from_address(write_handle_).resume();
    }

    void close() {
        shutdown(sockfd_, SHUT_RDWR);
        closed_ = true;
    }

    bool closed() const { return closed_; }

    size_t read_bytes() const {
        return read_buf_.bytes();
    }

    size_t write_bytes() const {
        return write_buf_.bytes();
    }

    util::NetInputStream get_input_stream() {
        return util::NetInputStream(&read_buf_);
    }

    util::NetOutputStream get_output_stream() {
        return util::NetOutputStream(&write_buf_);
    }

    ReadAwaiter async_read();
    WriteAwaiter async_write();

private:
    int sockfd_;              // peer socket
    std::string local_addr_;  // local address
    int local_port_;          // local port
    std::string peer_addr_;   // peer address
    int peer_port_;           // peer port

    bool closed_;             // socket closed

    util::InputBuffer read_buf_;    // read buffer
    util::OutputBuffer write_buf_;   // write buffer

    Executor* executor_;      // coroutine executor
    void* read_handle_;       // read coroutine handle
    void* write_handle_;       // write coroutine handle

    DISALLOW_COPY_AND_ASSIGN(Socket);
};

} // namespace xuanqiong::net
