#pragma once

#include <coroutine>
#include <unistd.h>
#include <sys/socket.h>

#include "util/common.h"
#include "util/iobuf.h"

namespace xuanqiong {
class Scheduler;
}

namespace xuanqiong::net {

struct ReadAwaiter {
    Scheduler* scheduler_;
    ReadAwaiter(Scheduler* scheduler) : scheduler_(scheduler) {}

    bool await_ready() { return false; }
    void await_suspend(std::coroutine_handle<> handle) {
        // scheduler_->schedule(handle);
    }
    void await_resume() {}
};

class Socket {
public:
    Socket(int fd) : sockfd_(fd) {}
    ~Socket() = default;

    // move only
    Socket(Socket&& other) {
        sockfd_ = other.sockfd_;
        other.sockfd_ = -1;
        buf_ = std::move(other.buf_);
        scheduler_ = other.scheduler_;
    }
    Socket& operator=(Socket&& other) {
        sockfd_ = other.sockfd_;
        other.sockfd_ = -1;
        buf_ = std::move(other.buf_);
        scheduler_ = other.scheduler_;
        return *this;
    }

    int fd() const { return sockfd_; }

    void close() {
        ::close(sockfd_);
    }

    ReadAwaiter read_data() {
        while (true) {
            int read_num = read(sockfd_, &buf_, sizeof(buf_));
            if (read_num > 0) {
                // 数据读取成功，处理数据
                info("read data, read_num: {}", read_num);
            } else if (read_num == 0) {
                info("read data, read_num = 0");
                // 对端关闭连接
                break;
            } else {
                info("read data, errno: {}", errno);
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    
                    return {scheduler_};
                }
            }
        }
        return {scheduler_};
    }

private:
    int sockfd_;
    util::IOBuf buf_;
    Scheduler* scheduler_;

    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
};

} // namespace xuanqiong::net
