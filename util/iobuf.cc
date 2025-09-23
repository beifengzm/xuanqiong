#include <cstring>

#include "util/common.h"
#include "util/iobuf.h"

namespace xuanqiong::util {

int IOBuf::read_from(int fd) {
    int nread = read(fd, buf_ + offset_, sizeof(buf_) - offset_);
    if (nread > 0) {
        offset_ += nread;
    }
    return nread;
}

void IOBuf::append(const char* data, size_t size) {
    memcpy(buf_ + offset_, data, size);
    offset_ += size;
}

} // namespace xuanqiong::util
