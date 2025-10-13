#include "util/output_stream.h"

namespace xuanqiong::util {

OutputBuffer::OutputBuffer() : to_write_bytes_(0) {
    cur_block_ = new BufferBlock;
    last_block_ = cur_block_;
}

bool OutputBuffer::next(void** data, int* size) {
    *data = last_block_->data + last_block_->end;
    *size = kBlockSize - last_block_->end;
    to_write_bytes_ += *size;
    last_block_->end += *size;
    if (last_block_->end == kBlockSize) {
        last_block_->next = new BufferBlock;
        last_block_ = last_block_->next;
    }
    return true;
}

void OutputBuffer::back_up(int count) {
    int to_backup = std::min(count, last_block_->end);
    to_write_bytes_ -= to_backup;
    last_block_->end -= to_backup;
}

int OutputBuffer::write_to(int fd) {
    int nwrite = 0;
    BufferBlock* block = cur_block_;
    while(cur_block_->begin != cur_block_->end) {
        int written = write(fd,
            block->data + block->begin, block->end - block->begin);
        if (written < 0) {
            break;
        }
        nwrite += written;
        block->begin += written;
        if (block->begin == kBlockSize) {
            auto next_block = block->next;
            delete block;
            block = next_block;
        }
    }
    return nwrite;
}

} // namespace xuanqiong::util
