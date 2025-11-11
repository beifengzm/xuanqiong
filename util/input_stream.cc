#include <unistd.h>

#include "util/input_stream.h"

namespace xuanqiong::util {

InputBuffer::InputBuffer() : read_bytes_(0), consumed_bytes_(0) {
    first_block_ = new BufferBlock;
    cur_block_ = first_block_;
    last_block_ = first_block_;
}

InputBuffer::~InputBuffer() {
    while (first_block_) {
        auto block = first_block_;
        first_block_ = first_block_->next;
        delete block;
    }
}

int InputBuffer::read_from(int fd) {
    int nread = ::read(fd, last_block_->data + last_block_->end,
        kBlockSize - last_block_->end);
    if (nread > 0) {
        last_block_->end += nread;
        read_bytes_ += nread;
        // if the current block is full, alloc a new block
        if (last_block_->end == kBlockSize) {
            last_block_->next = new BufferBlock;
            last_block_ = last_block_->next;
        }
    }
    return nread;
}

bool InputBuffer::fetch_uint32(uint32_t* value) {
    if (sizeof(uint32_t) > read_bytes_) {
        return false;
    }
    size_t copy_len = std::min(cur_block_->end - cur_block_->begin, (int)sizeof(uint32_t));
    memcpy(value, cur_block_->data + cur_block_->begin, copy_len);
    // info("begin: {}, end: {}, copy_len: {}", cur_block_->begin, cur_block_->end, copy_len);
    cur_block_->begin += copy_len;
    if (copy_len < sizeof(uint32_t)) {
        cur_block_ = cur_block_->next;
        memcpy(value + copy_len,
            cur_block_->data + cur_block_->begin, sizeof(uint32_t) - copy_len);
        cur_block_->begin += sizeof(uint32_t) - copy_len;
    }
    if (cur_block_->begin == kBlockSize && cur_block_->next) {
        cur_block_ = cur_block_->next;
    }
    consumed_bytes_ += sizeof(int32_t);
    read_bytes_ -= sizeof(int32_t);
    return true;
}

bool InputBuffer::next(const void** data, int* size) {
    if (cur_block_->begin == cur_block_->end || limit_ == 0) {
        return false;
    }
    *data = cur_block_->data + cur_block_->begin;
    *size = std::min(cur_block_->end - cur_block_->begin, limit_);
    limit_ -= limit_ == INT32_MAX ? 0 : *size;
    cur_block_->begin += *size;
    consumed_bytes_ += *size;
    if (cur_block_->begin == kBlockSize && cur_block_->next) {
        cur_block_ = cur_block_->next;
    }
    read_bytes_ -= *size;
    return true;
}

void InputBuffer::back_up(int n) {
    int backup_bytes = std::min(n, cur_block_->begin);
    cur_block_->begin -= backup_bytes;
    read_bytes_ += backup_bytes;
    limit_ += limit_ == INT32_MAX ? 0 : backup_bytes;
    consumed_bytes_ -= backup_bytes;

    // dealloc consumed blocks
    while (first_block_ != cur_block_) {
        auto block = first_block_;
        first_block_ = first_block_->next;
        delete block;
    }
}

bool InputBuffer::skip(int n) {
    if (n > byte_count()) {
        return false;
    }
    while (n > 0) {
        int to_skip = std::min(n, cur_block_->end - cur_block_->begin);
        cur_block_->begin += to_skip;
        n -= to_skip;
        consumed_bytes_ += to_skip;
        read_bytes_ -= to_skip;

        if (cur_block_->begin == kBlockSize && cur_block_->next) {
            cur_block_ = cur_block_->next;
        }
    }
    return true;
}

} // namespace xuanqiong::util
