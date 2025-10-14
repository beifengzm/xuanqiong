#pragma once

#include "util/common.h"
#include "util/buffer_block.h"
#include "google/protobuf/io/zero_copy_stream.h"

namespace xuanqiong::util {

class OutputBuffer {
public:
    OutputBuffer();
    ~OutputBuffer();

    // write data to fd, use writev
    int write_to(int fd);

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
