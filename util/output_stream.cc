#include <unistd.h>
#include <vector>

#include "util/output_stream.h"

namespace xuanqiong::util {

OutputBuffer::OutputBuffer() : to_write_bytes_(0), total_bytes_(0) {
    cur_block_ = new BufferBlock;
    last_block_ = cur_block_;
}

OutputBuffer::~OutputBuffer() {
    while (cur_block_) {
        auto next_block = cur_block_->next;
        delete cur_block_;
        cur_block_ = next_block;
    }
}

bool OutputBuffer::next(void** data, int* size) {
    if (last_block_->end == kBlockSize) {
        last_block_->next = new BufferBlock;
        last_block_ = last_block_->next;
        // info("[next] new block");
    }
    *data = last_block_->data + last_block_->end;
    *size = kBlockSize - last_block_->end;
    to_write_bytes_ += *size;
    total_bytes_ += *size;
    last_block_->end += *size;
    return true;
}

void OutputBuffer::back_up(int count) {
    int to_backup = std::min(count, last_block_->end);
    to_write_bytes_ -= to_backup;
    total_bytes_ -= to_backup;
    last_block_->end -= to_backup;
}

void OutputBuffer::append(const void* data, int size) {
    const char* ptr = (const char*)data;
    while (size > 0) {
        int to_write = std::min(size, kBlockSize - last_block_->end);
        memcpy(last_block_->data + last_block_->end, ptr, to_write);
        last_block_->end += to_write;
        ptr += to_write;
        size -= to_write;
        to_write_bytes_ += to_write;
        if (last_block_->end == kBlockSize) {
            last_block_->next = new BufferBlock;
            last_block_ = last_block_->next;
        }
    }
}

std::vector<iovec> OutputBuffer::get_iovecs() {
    std::vector<iovec> iovs;
    for (auto block = cur_block_; block && iovs.size() < IOV_MAX; block = block->next) {
        void* data = block->data + block->begin;
        size_t size = size_t(block->end - block->begin);
        iovs.emplace_back(data, size);
    }
    return iovs;
}

void OutputBuffer::write_add(int nwrite) {
    if (nwrite <= 0) return;
    int left = nwrite;
    to_write_bytes_ -= nwrite;
    while (left > 0) {
        int cur_write = std::min(left, cur_block_->end - cur_block_->begin);
        left -= cur_write;
        cur_block_->begin += cur_write;
        if (cur_block_->begin == kBlockSize) {
            if (!cur_block_->next) {
                last_block_->next = new BufferBlock;
                last_block_ = last_block_->next;
            }
            auto next_block = cur_block_->next;
            delete cur_block_;
            cur_block_ = next_block;
        }
    }
}

} // namespace xuanqiong::util
