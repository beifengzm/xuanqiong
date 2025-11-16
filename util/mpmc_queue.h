#pragma once

#include <vector>
#include <atomic>
#include <memory>
#include <thread>
#include <cassert>

#include "util/common.h"

namespace xuanqiong::util {

constexpr static int CACHE_LINE = 64;
constexpr static int CHUNK_SIZE = 4096;

template<typename T>
class MPMCQueue {
    struct Slot {
        alignas(CACHE_LINE) T value;
        std::atomic<bool> ready{false};
    };

    struct Chunk {
        alignas(CACHE_LINE) std::array<Slot, CHUNK_SIZE> slots;
        const size_t begin_index;
        std::atomic<Chunk*> next{nullptr};
        std::atomic<int> read_count{0};
        std::atomic<int> ref_count{0};

        Chunk(size_t begin_index) : begin_index(begin_index) {}
        ~Chunk() = default;
    };

public:
    MPMCQueue() {
        auto* init_chunk = new Chunk(0);
        head_chunk_.store(init_chunk, std::memory_order_relaxed);
        tail_chunk_.store(init_chunk, std::memory_order_relaxed);
        head_index_.store(0, std::memory_order_relaxed);
        tail_index_.store(0, std::memory_order_relaxed);
    }

    ~MPMCQueue() {
        Chunk* curr = head_chunk_.load(std::memory_order_relaxed);
        while (curr) {
            Chunk* next = curr->next.load(std::memory_order_relaxed);
            delete curr;
            curr = next;
        }
    }

    bool push(auto&& value);

    bool pop(T& value);

private:
    alignas(CACHE_LINE) std::atomic<Chunk*> head_chunk_;
    alignas(CACHE_LINE) std::atomic<size_t> head_index_;

    alignas(CACHE_LINE) std::atomic<Chunk*> tail_chunk_;
    alignas(CACHE_LINE) std::atomic<size_t> tail_index_;

    DISALLOW_COPY_AND_ASSIGN(MPMCQueue);
};

#include "util/mpmc_queue.inl.h"

} // namespace xuanqiong::util
