#pragma once

#include <google/protobuf/io/zero_copy_stream.h>

#include "util/common.h"
#include "util/buffer_block.h"

namespace xuanqiong::util {

class InputBuffer {
public:
    InputBuffer();
    ~InputBuffer();

    int read_from(int fd);

    // readable total data size in bytes
    size_t bytes() const { return read_bytes_; }

    bool fetch_uint32(uint32_t* value);

private:
    friend class NetInputStream;

    bool next(const void** data, int* size);

    void back_up(int n);

    bool skip(int n);

    void push_limit(int limit) {
        limit_ = limit;
    }

    void pop_limit() {
        limit_ = INT32_MAX;
    }

    int64_t byte_count() const { return consumed_bytes_; }

    size_t read_bytes_;       // size read from fd
    size_t consumed_bytes_;   // size consumed by ZeroCopyInputStream

    int limit_ = INT32_MAX;

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

    // fetch uint32_t from input stream
    bool fetch_uint32(uint32_t* value) {
        return input_buffer_->fetch_uint32(value);
    }

    void push_limit(int limit) {
        input_buffer_->push_limit(limit);
    }

    void pop_limit() {
        input_buffer_->pop_limit();
    }

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
