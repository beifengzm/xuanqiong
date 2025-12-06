#pragma once

#include <liburing.h>

#include "util/input_stream.h"
#include "util/output_stream.h"
#include "net/connection.h"

namespace xuanqiong {
class Executor;
}

namespace xuanqiong::net {

class UringConnection final : public Connection {
public:
    UringConnection(int fd, Executor* executor, bool dummy = false);

    virtual ~UringConnection();

    Executor* executor() const override { return executor_; }

    void send_add(int n);

    bool is_writing() const { return is_writing_; }

    // async read/write
    ReadAwaiter async_read() final override;
    WriteAwaiter async_write() final override;

private:
    bool dummy_;               // dummy connection, for event notify
    Executor* executor_;       // coroutine executor
    io_uring* uring_;          // io_uring instance

    bool is_writing_;          // is writing by io_uring_prep_writev
    std::vector<iovec> ioves_; // iovecs for write

    DISALLOW_COPY_AND_ASSIGN(UringConnection);
};

} // namespace xuanqiong::net
