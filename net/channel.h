#pragma once

#include <coroutine>
#include <unistd.h>

#include "util/common.h"
#include "util/iobuf.h"
#include "scheduler/awaitable.h"

namespace xuanqiong {
class Executor;
}

namespace xuanqiong::net {

class Channel {
public:
    Channel(int fd, Executor* executor);
    ~Channel();

    int fd() const { return sockfd_; }
    const std::string& local_addr() const { return local_addr_; }
    int local_port() const { return local_port_; }
    const std::string& peer_addr() const { return peer_addr_; }
    int peer_port() const { return peer_port_; }

    void close() {
        shutdown(sockfd_, SHUT_RDWR);
    }

    size_t read_bytes() const {
        return read_buf_.bytes();
    }

    size_t write_bytes() const {
        return write_buf_.bytes();
    }

    ReadAwaiter async_read();

private:
    int sockfd_;              // peer socket
    std::string local_addr_;  // local address
    int local_port_;          // local port
    std::string peer_addr_;   // peer address
    int peer_port_;           // peer port

    util::IOBuf read_buf_;    // read buffer
    util::IOBuf write_buf_;   // write buffer

    Executor* executor_;      // coroutine executor

    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;
};

} // namespace xuanqiong::net
