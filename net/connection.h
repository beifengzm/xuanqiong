#pragma once

#include <coroutine>

#include "net/socket.h"
#include "util/input_stream.h"
#include "util/output_stream.h"

namespace xuanqiong::net {

class Connection {
public:
    Connection(int fd) : socket_(std::make_unique<Socket>(fd)) {}
    ~Connection() = default;

    virtual ReadAwaiter async_read() = 0;
    virtual WriteAwaiter async_write() = 0;

    virtual Executor* executor() const = 0;

    int fd() const { return socket_->fd(); }
    bool closed() const { return socket_->closed(); }

    void close() { socket_->close(); }

    Socket* socket() const { return socket_.get(); }

    // set read/write coroutine handle
    void set_read_handle(void* handle) { read_handle_ = handle; }
    void set_write_handle(void* handle) { write_handle_ = handle; }

    // resume read/write coroutine handle
    void resume_read() const {
        std::coroutine_handle<>::from_address(read_handle_).resume();
    }
    void resume_write() const {
        std::coroutine_handle<>::from_address(write_handle_).resume();
    }

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

protected:
    util::InputBuffer read_buf_;    // read buffer
    util::OutputBuffer write_buf_;   // write buffer

    void* read_handle_;       // read coroutine handle
    void* write_handle_;       // write coroutine handle

    std::unique_ptr<Socket> socket_;  // socket

    DISALLOW_COPY_AND_ASSIGN(Connection);
};

} // namespace xuanqiong::net
