#pragma once

#include <cstddef>
#include <cstdint>

namespace xuanqiong::util {

constexpr static int kBlockSize = 13;

struct BufferBlock {
    int begin = 0;
    int end = 0;
    uint8_t data[kBlockSize];
    BufferBlock* next = nullptr;
};

} // namespace xuanqiong::util
