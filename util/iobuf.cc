#include <cstring>
#include <unistd.h>

#include "util/common.h"
#include "util/iobuf.h"

namespace xuanqiong::util {

int IOBuf::read_from(int fd) {
    int nread = ::read(fd, buf_ + end, sizeof(buf_) - end);
    if (nread > 0) {
        end += nread;
    }
    return nread;
}

int IOBuf::write_to(int fd) {
    int nwrite = ::write(fd, buf_ + begin, bytes());
    if (nwrite > 0) {
        begin += nwrite;
    }
    return nwrite;
}

void IOBuf::append(const char* data, size_t size) {
    memcpy(buf_ + end, data, size);
    end += size;
}

} // namespace xuanqiong::util
