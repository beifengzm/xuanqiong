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
    auto iovs = write_buf_.get_iovecs();

    auto sqe = io_uring_get_sqe(uring_);
    if (!sqe) {
        io_uring_submit(uring_);
        sqe = io_uring_get_sqe(uring_);
    }
    io_uring_prep_writev(sqe, fd(), iovs.data(), iovs.size(), 0);
    sqe->user_data =
        (static_cast<uint64_t>(EventType::WRITE) << 56) | reinterpret_cast<uint64_t>(this);
    return {this, true};
}

} // namespace xuanqiong::net

#endif
