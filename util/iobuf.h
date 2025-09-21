#pragma once
#include <cstddef>
#include <cstring>

namespace xuanqiong::util {

class IOBuf {
public:
    IOBuf() = default;
    ~IOBuf() = default;

    // move only
    IOBuf(IOBuf&& other) {
        offset_ = other.offset_;
        other.offset_ = -1;
        memcpy(buf_, other.buf_, offset_);
    }
    IOBuf& operator=(IOBuf&& other) {
        offset_ = other.offset_;
        other.offset_ = 0;
        memcpy(buf_, other.buf_, offset_);
        return *this;
    }

    size_t size() const { return sizeof(buf_); }
    char* data() { return buf_; }
    const char* data() const { return buf_; }

    void append(const char* data, size_t size);

private:
    char buf_[4096];
    size_t offset_ = 0;

    IOBuf(const IOBuf&) = delete;
    IOBuf& operator=(const IOBuf&) = delete;
};

} // namespace xuanqiong::util
