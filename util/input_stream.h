#pragma once

#include "util/common.h"
#include "util/buffer_block.h"
#include "google/protobuf/io/zero_copy_stream.h"

namespace xuanqiong::util {

class InputBuffer {
    friend class NetInputStream;

public:
    InputBuffer();
    ~InputBuffer() = default;

    int read_from(int fd);

    // TODO: error
    // valid read data size in bytes
    size_t byte_count() const { return read_bytes_; }

private:
    bool next(const void** data, int* size);

    void back_up(int n);

    bool skip(int n);

    size_t read_bytes_;

    // range [first_block, cur_block) for backup
    BufferBlock* first_block_;
    BufferBlock* cur_block_;
    BufferBlock* last_block_;

    DISALLOW_COPY_AND_ASSIGN(InputBuffer);
};

class NetInputStream : public google::protobuf::io::ZeroCopyInputStream {
public:
    NetInputStream(InputBuffer* input_buffer) : input_buffer_(input_buffer) {}
    ~NetInputStream() = default;

    bool Next(const void** data, int* size) override {
        return input_buffer_->next(data, size);
    }

    void BackUp(int count) override {
        input_buffer_->back_up(count);
    }

    bool Skip(int count) override {
        return input_buffer_->skip(count);
    }

    int64_t ByteCount() const override {
        return input_buffer_->byte_count();
    }

private:
    InputBuffer* input_buffer_;

    DISALLOW_COPY_AND_ASSIGN(NetInputStream);
};

} // namespace xuanqiong::util
