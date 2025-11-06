#include <gtest/gtest.h>
#include <atomic>
#include <thread>
#include <vector>
#include <chrono>

#include "util/mpmc_queue.h"

// Single Producer, Single Consumer (Basic Functionality)
TEST(MPMCQueueTest, SingleProducerSingleConsumer) {
    constexpr int kCapacity = 4;
    xuanqiong::util::MPMCQueue<int> queue(kCapacity);

    // Push until full
    EXPECT_TRUE(queue.push(10));
    EXPECT_TRUE(queue.push(20));
    EXPECT_TRUE(queue.push(30));
    EXPECT_TRUE(queue.push(40));
    EXPECT_FALSE(queue.push(50));  // Full

    // Pop and validate
    int val;
    EXPECT_TRUE(queue.pop(val) && val == 10);
    EXPECT_TRUE(queue.pop(val) && val == 20);
    EXPECT_TRUE(queue.pop(val) && val == 30);
    EXPECT_TRUE(queue.pop(val) && val == 40);
    EXPECT_FALSE(queue.pop(val));  // Empty
}

// Non-Trivial Type (std::string)
TEST(MPMCQueueTest, NonTrivialType_String) {
    constexpr int kCapacity = 8;
    xuanqiong::util::MPMCQueue<std::string> queue(kCapacity);

    ASSERT_TRUE(queue.push("Hello"));
    ASSERT_TRUE(queue.push("MPMC"));
    ASSERT_TRUE(queue.push("Queue"));

    std::string val;
    ASSERT_TRUE(queue.pop(val) && val == "Hello");
    ASSERT_TRUE(queue.pop(val) && val == "MPMC");
    ASSERT_TRUE(queue.pop(val) && val == "Queue");
    ASSERT_FALSE(queue.pop(val));  // Empty
}

// Multiple Producers, Single Consumer
TEST(MPMCQueueTest, MultipleProducersSingleConsumer) {
    constexpr int kCapacity = 16;
    constexpr int kNumProducers = 4;
    constexpr int kItemsPerProducer = 1000;

    xuanqiong::util::MPMCQueue<int> queue(kCapacity);
    std::atomic<int> total_popped{0};
    std::atomic<bool> start{false};

    // Producer threads
    std::vector<std::thread> producers;
    for (int i = 0; i < kNumProducers; ++i) {
        producers.emplace_back([&, i]() {
            while (!start.load(std::memory_order_relaxed)) {
                std::this_thread::yield();
            }
            for (int j = 0; j < kItemsPerProducer; ++j) {
                int val = i * 1000 + j;
                while (!queue.push(val)) {
                    std::this_thread::yield();  // queue full
                }
            }
        });
    }

    // Consumer thread
    std::thread consumer([&]() {
        while (!start.load(std::memory_order_relaxed)) {
            std::this_thread::yield();
        }
        int popped = 0;
        while (popped < kNumProducers * kItemsPerProducer) {
            int val;
            if (queue.pop(val)) {
                ++popped;
            } else {
                std::this_thread::yield();  // queue empty
            }
        }
        total_popped = popped;
    });

    // Start all threads
    start.store(true, std::memory_order_relaxed);

    // Join
    for (auto& t : producers) t.join();
    consumer.join();

    EXPECT_EQ(total_popped, kNumProducers * kItemsPerProducer);
}

// Multiple Producers, Multiple Consumers
TEST(MPMCQueueTest, MultipleProducersMultipleConsumers) {
    constexpr int kCapacity = 8;  // Small to trigger contention
    constexpr int kNumProducers = 4;
    constexpr int kNumConsumers = 4;
    constexpr int kItemsPerProducer = 10000;

    xuanqiong::util::MPMCQueue<int> queue(kCapacity);
    std::atomic<int> total_pushed{0};
    std::atomic<int> total_popped{0};
    std::atomic<bool> start{false};

    // Producers
    std::vector<std::thread> producers;
    for (int i = 0; i < kNumProducers; ++i) {
        producers.emplace_back([&, i]() {
            while (!start.load(std::memory_order_relaxed)) {
                std::this_thread::yield();
            }
            for (int j = 0; j < kItemsPerProducer; ++j) {
                int val = i * 100000 + j;
                while (!queue.push(val)) {
                    std::this_thread::yield();  // full
                }
                total_pushed++;
            }
        });
    }

    // Consumers
    std::vector<std::thread> consumers;
    for (int i = 0; i < kNumConsumers; ++i) {
        consumers.emplace_back([&]() {
            while (!start.load(std::memory_order_relaxed)) {
                std::this_thread::yield();
            }
            int val;
            while (total_popped < kNumProducers * kItemsPerProducer) {
                if (queue.pop(val)) {
                    total_popped++;
                } else {
                    std::this_thread::yield();  // empty
                }
            }
        });
    }

    // Start
    start.store(true, std::memory_order_relaxed);

    // Join
    for (auto& t : producers) t.join();
    for (auto& t : consumers) t.join();

    EXPECT_EQ(total_pushed, kNumProducers * kItemsPerProducer);
    EXPECT_EQ(total_popped, kNumProducers * kItemsPerProducer);
}

// Edge Case — Queue Capacity of 1
TEST(MPMCQueueTest, SingleCapacityQueue) {
    xuanqiong::util::MPMCQueue<int> queue(1);

    EXPECT_TRUE(queue.push(42));
    EXPECT_FALSE(queue.push(43));  // Full

    int val;
    EXPECT_TRUE(queue.pop(val) && val == 42);
    EXPECT_FALSE(queue.pop(val));  // Empty

    EXPECT_TRUE(queue.push(99));
    EXPECT_TRUE(queue.pop(val) && val == 99);
}

TEST(MPMCQueueTest, HighConcurrency_16P_16C) {
    constexpr int kCapacity = 32;
    constexpr int kNumProducers = 16;
    constexpr int kNumConsumers = 16;
    constexpr int kItemsPerProducer = 5000;

    xuanqiong::util::MPMCQueue<int> queue(kCapacity);
    std::atomic<int> total_popped{0};
    std::atomic<bool> start{false};

    std::vector<std::thread> producers;
    for (int i = 0; i < kNumProducers; ++i) {
        producers.emplace_back([&, i]() {
            while (!start.load()) { std::this_thread::yield(); }
            for (int j = 0; j < kItemsPerProducer; ++j) {
                int val = i * 10000 + j;
                while (!queue.push(val)) {
                    std::this_thread::yield();
                }
            }
        });
    }

    std::vector<std::thread> consumers;
    for (int i = 0; i < kNumConsumers; ++i) {
        consumers.emplace_back([&]() {
            while (!start.load()) { std::this_thread::yield(); }
            int val;
            while (total_popped < kNumProducers * kItemsPerProducer) {
                if (queue.pop(val)) {
                    ++total_popped;
                } else {
                    std::this_thread::yield();
                }
            }
        });
    }

    start.store(true);

    for (auto& t : producers) t.join();
    for (auto& t : consumers) t.join();

    ASSERT_EQ(total_popped, kNumProducers * kItemsPerProducer);
}

TEST(MPMCQueueTest, PushPopExactMatch) {
    constexpr int kCapacity = 128;
    constexpr int kTotalItems = 100000;

    xuanqiong::util::MPMCQueue<int> queue(kCapacity);
    std::atomic<int> popped{0};

    std::thread producer([&]() {
        for (int i = 0; i < kTotalItems; ++i) {
            while (!queue.push(i)) {
                std::this_thread::yield();
            }
        }
    });

    std::thread consumer([&]() {
        int val;
        while (popped < kTotalItems) {
            if (queue.pop(val)) {
                ++popped;
            } else {
                std::this_thread::yield();
            }
        }
    });

    producer.join();
    consumer.join();

    ASSERT_EQ(popped, kTotalItems);
}

TEST(MPMCQueueTest, MessageWithProducerId) {
    constexpr int kCapacity = 16;
    constexpr int kNumProducers = 4;
    constexpr int kItemsPerProducer = 1000;

    xuanqiong::util::MPMCQueue<std::pair<int, int>> queue(kCapacity);  // (producer_id, seq)
    std::atomic<int> total_received{0};

    std::vector<std::thread> producers;
    for (int i = 0; i < kNumProducers; ++i) {
        producers.emplace_back([&, i]() {
            for (int j = 0; j < kItemsPerProducer; ++j) {
                while (!queue.push(std::make_pair(i, j))) {
                    std::this_thread::yield();
                }
            }
        });
    }

    std::thread consumer([&]() {
        std::pair<int, int> val;
        while (total_received < kNumProducers * kItemsPerProducer) {
            if (queue.pop(val)) {
                EXPECT_GE(val.first, 0);
                EXPECT_LT(val.first, kNumProducers);
                EXPECT_GE(val.second, 0);
                EXPECT_LT(val.second, kItemsPerProducer);
                ++total_received;
            } else {
                std::this_thread::yield();
            }
        }
    });

    for (auto& t : producers) t.join();
    consumer.join();

    EXPECT_EQ(total_received, kNumProducers * kItemsPerProducer);
}
