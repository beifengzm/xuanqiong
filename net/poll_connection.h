#pragma once

#include "util/input_stream.h"
#include "util/output_stream.h"
#include "net/connection.h"

namespace xuanqiong {
class Executor;
}

namespace xuanqiong::net {

class PollConnection : public Connection {
public:
    PollConnection(int fd, Executor* executor, bool dummy = false);

    Executor* executor() const override { return executor_; }

    // async read/write
    ReadAwaiter async_read() override;
    WriteAwaiter async_write() override;

private:
    Executor* executor_;      // coroutine executor

    DISALLOW_COPY_AND_ASSIGN(PollConnection);
};

} // namespace xuanqiong::net
