template <typename T>
bool MPMCQueue<T>::push(auto&& value) {
    while (true) {
        auto tail_chunk = tail_chunk_.load(std::memory_order_acquire);
        auto tail_idx = tail_index_.load(std::memory_order_acquire);

        if (tail_index_.compare_exchange_weak(tail_idx, tail_idx + 1,
                std::memory_order_release, std::memory_order_relaxed)) {
            auto write_idx = tail_idx;
            auto write_chunk = tail_chunk;
            if (tail_idx >= tail_chunk->begin_index + CHUNK_SIZE) {
                auto next_chunk = new Chunk(tail_chunk->begin_index + CHUNK_SIZE);
                if (tail_chunk_.compare_exchange_strong(
                        tail_chunk, next_chunk,
                        std::memory_order_release,
                        std::memory_order_relaxed)) {
                    tail_chunk->next.store(next_chunk, std::memory_order_release);
                } else {
                    delete next_chunk;
                }
                while (write_chunk->begin_index + CHUNK_SIZE <= write_idx) {
                    while (!write_chunk->next.load(std::memory_order_acquire)) {}
                    write_chunk = write_chunk->next.load(std::memory_order_acquire);
                }
            }
            auto& write_slot = write_chunk->slots[write_idx - write_chunk->begin_index];
            write_slot.value = std::forward<decltype(value)>(value);
            write_slot.ready.store(true, std::memory_order_release);
            return true;
        }
    }
}

template <typename T>
bool MPMCQueue<T>::pop(T& value) {
    while (true) {
        auto head_chunk = head_chunk_.load(std::memory_order_acquire);
        auto head_idx = head_index_.load(std::memory_order_acquire);
        if (head_idx == tail_index_.load(std::memory_order_acquire)) {
            return false;  // empty
        }
        if (head_chunk->read_count.load(std::memory_order_acquire) == CHUNK_SIZE
                && head_chunk->ref_count.load(std::memory_order_acquire) == 0) {
            if (!head_chunk->next.load(std::memory_order_acquire)) continue;
            if (head_chunk_.compare_exchange_strong(
                head_chunk, head_chunk->next.load(std::memory_order_acquire),
                std::memory_order_release, std::memory_order_relaxed)) {
                delete head_chunk;
            }
            continue;
        }

        head_chunk->ref_count.fetch_add(1, std::memory_order_acquire);

        if (head_index_.compare_exchange_strong(head_idx, head_idx + 1,
                std::memory_order_acquire, std::memory_order_relaxed)) {
            auto read_idx = head_idx;
            auto read_chunk = head_chunk;
            while (read_chunk->begin_index + CHUNK_SIZE <= read_idx) {
                while (!read_chunk->next.load(std::memory_order_acquire)) {}
                read_chunk = read_chunk->next.load(std::memory_order_acquire);
            }
            if (read_chunk != head_chunk) {
                head_chunk->ref_count.fetch_sub(1, std::memory_order_release);
            }
            auto& slot = read_chunk->slots[read_idx - read_chunk->begin_index];
            while (!slot.ready.load(std::memory_order_acquire)) {}
            value = std::move(slot.value);
            read_chunk->read_count.fetch_add(1, std::memory_order_release);
            if (read_chunk == head_chunk) {
                head_chunk->ref_count.fetch_sub(1, std::memory_order_release);
            }
            return true;
        }
    }
}
