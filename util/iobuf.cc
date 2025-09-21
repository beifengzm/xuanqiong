#include <cstring>

#include "util/common.h"
#include "util/iobuf.h"

namespace xuanqiong::util {

void IOBuf::append(const char* data, size_t size) {
    memcpy(buf_ + offset_, data, size);
    offset_ += size;
}

} // namespace xuanqiong::util
