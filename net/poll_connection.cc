#include "net/poll_connection.h"

namespace xuanqiong::net {

PollConnection::PollConnection(int fd, Executor* executor, bool dummy) :
    Connection(fd, dummy), executor_(executor) {
}

ReadAwaiter PollConnection::async_read() {
    auto [buffer, max_size] = read_buf_.get_buffer();
    int read_bytes = 0;
    while (max_size > 0) {
        int n = ::read(socket_->fd(), buffer, max_size);
        if (n > 0) {
            read_bytes += n;
            buffer += n;
            max_size -= n;
            continue;
        } else if (n == 0) {
            close();
            break;
        } else {
            // retry
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            if (errno == EINTR) {
                continue;
            }
            error("read data, errno: {}", errno);
            close();
            break;
        }
    }
    if (closed()) {
        resume_write();
    }
    read_buf_.recv_add(read_bytes);
    bool should_suspend = !closed() && read_bytes <= 0;
    return {this, should_suspend};
}

WriteAwaiter PollConnection::async_write() {
    int need_write = write_buf_.bytes();
    int nwrite = 0;
    // info("[start] async_write, need_write: {}", need_write);
    while (nwrite < need_write) {
        auto iovs = write_buf_.get_iovecs();
        int n = ::writev(fd(), iovs.data(), iovs.size());
        if (n >= 0) {
            nwrite += n;
            continue;
        } else {
            // retry
            if (errno == EINTR) {
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            // other error
            error("write data, errno: {}", errno);
            break;
        }
    }
    write_buf_.write_add(nwrite);
    bool should_suspend = !closed() && nwrite < need_write;
    return {this, should_suspend};
}

} // namespace xuanqiong::net
