#ifdef __linux__

#include "net/uring_connection.h"
#include "scheduler/uring_executor.h"

namespace xuanqiong::net {

UringConnection::UringConnection(int fd, Executor* executor, bool dummy)
    : Connection(fd, dummy), executor_(executor) {
    uring_ = static_cast<UringExecutor*>(executor)->uring();
}

UringConnection::~UringConnection() = default;

ReadAwaiter UringConnection::async_read() {
    auto [buffer, max_size] = read_buf_.get_buffer();

    auto sqe = io_uring_get_sqe(uring_);
    if (!sqe) {
        io_uring_submit(uring_);
        sqe = io_uring_get_sqe(uring_);
    }
    io_uring_prep_read(sqe, fd(), buffer, max_size, 0);
    sqe->user_data =
        (static_cast<uint64_t>(EventType::READ) << 56) | reinterpret_cast<uint64_t>(this);

    return {this, true};
}

WriteAwaiter UringConnection::async_write() {
    int need_write = write_buf_.bytes();
    int nwrite = 0;
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
    // if (nwrite == 0) {
    //     info("[async_write] nwrite: {}, need_write: {}", nwrite, need_write);
    // }
    write_buf_.write_add(nwrite);
    bool should_suspend = !closed() && nwrite < need_write;
    return {this, should_suspend};
}

} // namespace xuanqiong::net

#endif
