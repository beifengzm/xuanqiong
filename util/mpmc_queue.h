#pragma once

#include <vector>
#include <atomic>
#include <assert.h>

#include "util/common.h"

namespace xuanqiong::util {

constexpr static int CACHE_LINE = 64;

template<typename T>
class MPMCQueue {
    struct Slot {
        alignas(CACHE_LINE) T value;
        std::atomic<bool> ready{false};
    };

public:
    MPMCQueue(int capacity)
        : capacity_(capacity), mask_(capacity - 1), data_(capacity) {
        // capacity must be power of 2
        assert((capacity & (capacity - 1)) == 0);
    }
    ~MPMCQueue() = default;

    bool push(auto&& value);

    bool pop(T& value);

private:
    const size_t capacity_;
    const size_t mask_;
    std::vector<Slot> data_;

    alignas(CACHE_LINE) std::atomic<size_t> head_{0};
    alignas(CACHE_LINE) std::atomic<size_t> tail_{0};

    DISALLOW_COPY_AND_ASSIGN(MPMCQueue);
};

#include "util/mpmc_queue.inl.h"

} // namespace xuanqiong::util
