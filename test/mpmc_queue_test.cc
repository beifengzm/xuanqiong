#include <gtest/gtest.h>
#include <atomic>
#include <thread>
#include <vector>
#include <chrono>

#include "util/mpmc_queue.h"

// Single Producer, Single Consumer (Basic Functionality)
TEST(MPMCQueueTest, SingleProducerSingleConsumer) {
    xuanqiong::util::MPMCQueue<int> queue;

    // Push until full
    for (int i = 0; i < 1000; ++i) {
        EXPECT_TRUE(queue.push(i));
    }

    // Pop and validate
    int val;
    for (int i = 0; i < 1000; ++i) {
        EXPECT_TRUE(queue.pop(val) && val == i);
    }
    EXPECT_FALSE(queue.pop(val));  // Empty
}

// Non-Trivial Type (std::string)
TEST(MPMCQueueTest, NonTrivialType_String) {
    xuanqiong::util::MPMCQueue<std::string> queue;

    for (int i = 0; i < 1000; ++i) {
        ASSERT_TRUE(queue.push("string_" + std::to_string(i)));
    }

    std::string val;
    for (int i = 0; i < 1000; ++i) {
        ASSERT_TRUE(queue.pop(val) && val == "string_" + std::to_string(i));
    }
    ASSERT_FALSE(queue.pop(val));  // Empty
}

// Multiple Producers, Single Consumer
TEST(MPMCQueueTest, MultipleProducersSingleConsumer) {
    constexpr int kNumProducers = 4;
    constexpr int kItemsPerProducer = 1000;

    xuanqiong::util::MPMCQueue<int> queue;
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
    // constexpr int kNumProducers = 4;
    // constexpr int kNumConsumers = 4;
    constexpr int kNumProducers = 1;
    constexpr int kNumConsumers = 2;
    constexpr int kItemsPerProducer = 10000;

    xuanqiong::util::MPMCQueue<int> queue;
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

TEST(MPMCQueueTest, HighConcurrency_16P_16C) {
    constexpr int kNumProducers = 16;
    constexpr int kNumConsumers = 16;
    constexpr int kItemsPerProducer = 5000;

    xuanqiong::util::MPMCQueue<int> queue;
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
    constexpr int kTotalItems = 100000;

    xuanqiong::util::MPMCQueue<int> queue;
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
                // std::this_thread::yield();
            }
        }
    });

    producer.join();
    consumer.join();

    ASSERT_EQ(popped, kTotalItems);
}

TEST(MPMCQueueTest, MessageWithProducerId) {
    constexpr int kNumProducers = 4;
    constexpr int kItemsPerProducer = 1000;

    xuanqiong::util::MPMCQueue<std::pair<int, int>> queue;  // (producer_id, seq)
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
            }
        }
    });

    for (auto& t : producers) t.join();
    consumer.join();

    EXPECT_EQ(total_received, kNumProducers * kItemsPerProducer);
}

TEST(MPMCQueueTest, MultiProducerMultiConsumer) {
    constexpr int num_producers = 16;
    constexpr int num_consumers = 16;
    constexpr int items_per_producer = 10000;

    xuanqiong::util::MPMCQueue<int> queue;

    std::atomic<uint64_t> sum{0};
    std::atomic<int> producers_done{0};
    std::atomic<int> consumers_should_stop{0};

    std::vector<std::thread> consumers;
    for (int i = 0; i < num_consumers; ++i) {
        consumers.emplace_back([&queue, &sum, &producers_done, &consumers_should_stop, num_producers]() {
            int local_sum = 0;
            while (true) {
                int value;
                if (queue.pop(value)) {
                    local_sum += value;
                } else {
                    if (producers_done.load(std::memory_order_acquire) == num_producers) {
                        while (queue.pop(value)) {
                            local_sum += value;
                        }
                        break;
                    }
                    std::this_thread::yield();
                }
            }
            sum.fetch_add(local_sum, std::memory_order_relaxed);
        });
    }

    std::vector<std::thread> producers;
    for (int i = 0; i < num_producers; ++i) {
        producers.emplace_back([&queue, i, items_per_producer]() {
            int base = i * items_per_producer;
            for (int j = 0; j < items_per_producer; ++j) {
                while (!queue.push(base + j)) {
                    std::this_thread::yield();
                }
            }
        });
    }

    for (auto& t : producers) {
        t.join();
    }
    producers_done.store(num_producers, std::memory_order_release);

    for (auto& t : consumers) {
        t.join();
    }

    uint64_t total_items = static_cast<uint64_t>(num_producers) * items_per_producer;
    uint64_t expected_sum = (total_items - 1) * total_items / 2;

    EXPECT_EQ(sum.load(std::memory_order_relaxed), expected_sum);
}
