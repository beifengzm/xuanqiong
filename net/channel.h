#pragma once

#include <coroutine>
#include <unistd.h>
#include <sys/socket.h>

#include "util/common.h"
#include "util/iobuf.h"

namespace xuanqiong {
class Executor;
}

namespace xuanqiong::net {

struct ReadAwaiter {
    int fd;
    bool closed;
    Executor* executor;

    bool await_ready() { return closed; }
    void await_suspend(std::coroutine_handle<> handle) {
        executor->schedule({fd, handle});
    }
    bool await_resume() { return closed; }
};

class Channel {
public:
    Channel(int fd) : sockfd_(fd) {}
    ~Channel() = default;

    // move only
    Channel(Channel&& other) {
        sockfd_ = other.sockfd_;
        other.sockfd_ = -1;
        read_buf_ = std::move(other.read_buf_);
    }
    Channel& operator=(Channel&& other) {
        sockfd_ = other.sockfd_;
        other.sockfd_ = -1;
        read_buf_ = std::move(other.read_buf_);
        return *this;
    }

    int fd() const { return sockfd_; }

    void close() {
        ::close(sockfd_);
    }

    ReadAwaiter async_read() {
        while (true) {
            int nread = read_buf_.read_from(sockfd_);
            if (nread > 0) {
                continue;
            } else if (nread == 0) {
                info("connection closed by perr");
                ::close(sockfd_);
                return {sockfd_, true, executor_};
            } else {
                // retry
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    return {sockfd_, false, executor_};
                }
                if (errno == EINTR) {
                    continue;
                }
                error("read data, errno: {}", errno);
                // read error, close connection
                ::close(sockfd_);
                return {sockfd_, false, executor_};
            }
        }
    }

private:
    int sockfd_;              // peer socket
    util::IOBuf read_buf_;    // read buffer
    util::IOBuf write_buf_;   // write buffer
    Executor* executor_;      // coroutine executor

    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;
};

} // namespace xuanqiong::net
