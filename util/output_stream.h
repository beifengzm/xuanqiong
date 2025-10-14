#pragma once

#include "util/common.h"
#include "util/buffer_block.h"
#include "google/protobuf/io/zero_copy_stream.h"

namespace xuanqiong::util {

class OutputBuffer {
    friend class NetOutputStream;

public:
    OutputBuffer();
    ~OutputBuffer() = default;

    // write data to fd
    int write_to(int fd);

    void append(const void* data, int size);

    // TODO: error
    // data size in bytes to write
    int64_t byte_count() const { return to_write_bytes_; }

private:
    bool next(void** data, int* size);

    void back_up(int count);

    BufferBlock* cur_block_;
    BufferBlock* last_block_;

    int64_t to_write_bytes_;

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
