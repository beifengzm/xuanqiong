#pragma once

#include <liburing.h>

#include "util/input_stream.h"
#include "util/output_stream.h"
#include "net/connection.h"

namespace xuanqiong {
class Executor;
}

namespace xuanqiong::net {

class UringConnection : public Connection {
public:
    UringConnection(int fd, Executor* executor, bool dummy = false);

    virtual ~UringConnection();

    Executor* executor() const override { return executor_; }

    void write_add(int n);

    size_t back_left() const { return back_left_; }

    // async read/write
    ReadAwaiter async_read() override;
    WriteAwaiter async_write() override;

private:
    bool dummy_;              // dummy connection, for event notify
    Executor* executor_;      // coroutine executor
    io_uring* uring_;        // io_uring instance

    size_t back_left_;      // bytes left to back
    std::vector<iovec> ioves_; // iovecs for write

    DISALLOW_COPY_AND_ASSIGN(UringConnection);
};

} // namespace xuanqiong::net
