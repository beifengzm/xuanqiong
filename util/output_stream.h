#pragma once

#include <sys/uio.h>
#include <google/protobuf/io/zero_copy_stream.h>

#include "util/common.h"
#include "util/buffer_block.h"

namespace xuanqiong::util {

class OutputBuffer {
public:
    OutputBuffer();
    ~OutputBuffer();

    void append(const void* data, int size);

    // write data to fd, use writev
    // int write_to(int fd);
    std::vector<iovec> get_iovecs();
    void send_add(int send_bytes);

    // data size in bytes to write
    size_t bytes() const { return to_write_bytes_; }

private:
    friend class NetOutputStream;

    bool next(void** data, int* size);

    void back_up(int count);

    int64_t byte_count() const { return to_write_bytes_; }

    BufferBlock* cur_block_;
    BufferBlock* last_block_;

    size_t to_write_bytes_;    // size to write to fd
    size_t total_bytes_;       // total size from ZeroCopyOutputStream

    DISALLOW_COPY_AND_ASSIGN(OutputBuffer);
};

class NetOutputStream : public google::protobuf::io::ZeroCopyOutputStream {
public:
    NetOutputStream(OutputBuffer* output_buffer) : output_buffer_(output_buffer) {}
    ~NetOutputStream() = default;

    void append(const void* data, int size) {
        output_buffer_->append(data, size);
    }

    bool Next(void** data, int* size) override {
        return output_buffer_->next(data, size);
    }

    void BackUp(int count) override {
        output_buffer_->back_up(count);
    }

    int64_t ByteCount() const override {
        return output_buffer_->byte_count();
    }

private:
    OutputBuffer* output_buffer_;

    DISALLOW_COPY_AND_ASSIGN(NetOutputStream);
};

} // namespace xuanqiong::util
