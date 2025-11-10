template <typename T>
bool MPMCQueue<T>::push(auto&& value) {
    while (true) {
        auto head = head_.load(std::memory_order_acquire);
        auto tail = tail_.load(std::memory_order_acquire);
        if (tail - head >= capacity_) {  // full
            return false;
        }
        if (tail_.compare_exchange_weak(tail, tail + 1,
                    std::memory_order_release, std::memory_order_relaxed)) {
            auto& slot = data_[tail & mask_];
            while (slot.ready.load(std::memory_order_relaxed)) {
                std::this_thread::yield();
            }
            slot.value = std::forward<decltype(value)>(value);
            slot.ready.store(true, std::memory_order_release);
            return true;
        }
    }
}

template <typename T>
bool MPMCQueue<T>::pop(T& value) {
    while (true) {
        auto head = head_.load(std::memory_order_acquire);
        auto tail = tail_.load(std::memory_order_acquire);
        if (head == tail) {  // empty
            return false;
        }
        if (head_.compare_exchange_weak(head, head + 1,
                    std::memory_order_acquire, std::memory_order_relaxed)) {
            auto& slot = data_[head & mask_];
            while (!slot.ready.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            value = std::move(slot.value);
            slot.ready.store(false, std::memory_order_relaxed);
            return true;
        }
    }
}
