#ifdef __linux__

#include "net/uring_connection.h"
#include "scheduler/uring_executor.h"

namespace xuanqiong::net {

UringConnection::UringConnection(int fd, Executor* executor, bool dummy)
    : Connection(fd, dummy), executor_(executor), back_left_(0) {
    uring_ = static_cast<UringExecutor*>(executor)->uring();
}

UringConnection::~UringConnection() = default;

void UringConnection::write_add(int nwrite) {
    back_left_ -= nwrite;
    Connection::write_add(nwrite);
}

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
    // if there are bytes left to back, suspend
    if (back_left_ > 0) {
        return {this, true};
    }

    back_left_ = write_buf_.bytes();
    if (back_left_ == 0) {
        // no bytes to write, do not suspend
        return {this, false};
    }

    write_buf_.get_iovecs().swap(ioves_);

    auto sqe = io_uring_get_sqe(uring_);
    if (!sqe) {
        io_uring_submit(uring_);
        sqe = io_uring_get_sqe(uring_);
    }
    io_uring_prep_writev(sqe, fd(), ioves_.data(), ioves_.size(), 0);
    sqe->user_data =
        (static_cast<uint64_t>(EventType::WRITE) << 56) | reinterpret_cast<uint64_t>(this);

    bool should_suspend = !closed();
    return {this, should_suspend};
}

} // namespace xuanqiong::net

#endif
