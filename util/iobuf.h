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
        begin = other.begin;
        end = other.end;
        other.begin = 0;
        other.end = 0;
        memcpy(buf_, other.buf_, end - begin);
    }
    IOBuf& operator=(IOBuf&& other) {
        begin = other.begin;
        end = other.end;
        other.begin = 0;
        other.end = 0;
        memcpy(buf_, other.buf_, end - begin);
        return *this;
    }

    std::string_view fetch(size_t n) {
        n = std::min(n, bytes());
        std::string_view data(buf_ + begin, n);
        begin += n;
        return data;
    }

    size_t bytes() const { return end - begin; }   // valid data size in bytes

    int read_from(int fd);
    int write_to(int fd);

    void append(const char* data, size_t size);

private:
    char buf_[65536];
    size_t begin = 0;
    size_t end = 0;         // empty in end

    IOBuf(const IOBuf&) = delete;
    IOBuf& operator=(const IOBuf&) = delete;
};

} // namespace xuanqiong::util
