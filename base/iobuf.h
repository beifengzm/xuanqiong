#pragma once
#include <cstddef>

namespace xuanqiong::base {

class IOBuf {
public:
    IOBuf() = default;
    ~IOBuf() = default;

    size_t size() const { return sizeof(buf_); }
    char* data() { return buf_; }
    const char* data() const { return buf_; }

private:
    char buf_[1024];
};

} // namespace xuanqiong::base
