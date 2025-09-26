#pragma once

#include <coroutine>
#include <unistd.h>
#include <sys/socket.h>

#include "util/common.h"
#include "util/iobuf.h"
#include "scheduler/task.h"

namespace xuanqiong {
class Executor;
}

namespace xuanqiong::net {

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
        int nread = 0;
        while (true) {
            int n = read_buf_.read_from(sockfd_);
            if (n > 0) {
                nread += n;
                continue;
            } else if (n == 0) {
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
