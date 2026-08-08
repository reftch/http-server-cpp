#include "task.h"  // Assuming this is your header file name

#include <gtest/gtest.h>

#include <atomic>
#include <thread>

using namespace http;
using namespace std::chrono_literals;

// Test Case 1: Verify that a simple task actually executes
TEST(TaskQueueTest, ExecutesSimpleTask) {
    TaskQueue queue(2);
    std::atomic<bool> executed{false};

    queue.enqueue("test_key", [&]() {
        executed = true;
    });

    // Give it a moment to process
    std::this_thread::sleep_for(50ms);
    EXPECT_TRUE(executed.load());
}

// Test Case 2: Verify the unique key functionality (Deduplication)
TEST(TaskQueueTest, DeduplicatesSameKey) {
    TaskQueue queue(1);  // Single thread to make timing predictable
    std::atomic<int> call_count{0};
    std::atomic<bool> start_blocking{false};
    std::atomic<bool> unblock_task{false};

    // This task will sleep until we tell it to, holding the key "busy"
    queue.enqueue("busy_key", [&]() {
        call_count++;
        start_blocking = true;
        while (!unblock_task) {
            std::this_thread::sleep_for(10ms);
        }
    });

    // Wait until the first task is actually running
    while (!start_blocking) {
        std::this_thread::yield();
    }

    // Attempt to enqueue the SAME key again while it's running
    queue.enqueue("busy_key", [&]() {
        call_count++;
    });

    // Attempt to enqueue a DIFFERENT key (should work)
    queue.enqueue("other_key", [&]() {
        call_count++;
    });

    // Release the first task
    unblock_task = true;

    // Wait for completion
    std::this_thread::sleep_for(100ms);

    // Expected: 1 (the original) + 1 (the other_key) = 2.
    // The second "busy_key" should have been ignored.
    EXPECT_EQ(call_count.load(), 2);
}

// Test Case 3: Verify that once a task is done, the key is released
TEST(TaskQueueTest, ReleasesKeyAfterCompletion) {
    TaskQueue queue(1);
    std::atomic<int> counter{0};

    // Run task 1
    queue.enqueue("key", [&]() {
        counter++;
    });

    // Wait for it to finish
    while (counter == 0) std::this_thread::yield();
    std::this_thread::sleep_for(20ms);

    // Run task 2 with same key - should succeed now because key was erased
    queue.enqueue("key", [&]() {
        counter++;
    });

    std::this_thread::sleep_for(50ms);
    EXPECT_EQ(counter.load(), 2);
}

// Test Case 4: Verify multiple threads can work in parallel
TEST(TaskQueueTest, ParallelExecution) {
    const size_t thread_count = 4;
    TaskQueue queue(thread_count);
    std::atomic<int> active_tasks{0};
    // std::atomic<int> max_observed_parallelism{0};

    // We will submit tasks that stay "active" for a while
    for (size_t i = 0; i < thread_count * 2; ++i) {
        queue.enqueue("task_" + std::to_string(i), [&]() {
            active_tasks++;
            // Track highest number of concurrent tasks seen
            // int current = active_tasks.load();
            // This is a bit racey in the test itself, but good enough for logic
            std::this_thread::sleep_for(100ms);
            active_tasks--;
        });
    }

    // Since we don't have a way to easily track 'max' without more complex code,
    // we just ensure the queue doesn't deadlock and finishes.
    std::this_thread::sleep_for(500ms);
    EXPECT_EQ(active_tasks.load(), 0);
}

// Test Case 5: Stress test to check for race conditions in key management
TEST(TaskQueueTest, StressTestKeys) {
    TaskQueue queue(4);
    // std::atomic<bool> stop_stress{false};

    // Thread that constantly tries to enqueue tasks with various keys
    auto stress_func = [&]() {
        for (int i = 0; i < 100; ++i) {
            std::string key = "key_" + std::to_string(i % 5);  // Rotate through 5 keys
            queue.enqueue(key, []() {
                std::this_thread::sleep_for(1ms);
            });
        }
    };

    std::thread t1(stress_func);
    std::thread t2(stress_func);
    std::thread t3(stress_func);

    t1.join();
    t2.join();
    t3.join();

    // If we reached here without crashing or deadlocking, it's a pass
    SUCCEED();
}
