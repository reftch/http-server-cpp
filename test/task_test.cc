#include "task.h"  // Assuming this is your header file name

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>

using namespace http;
using namespace std::chrono_literals;

class TaskQueueTest : public ::testing::Test {
   protected:
    // Helper to wait for a condition to become true (with timeout)
    void waitForCondition(std::function<bool()> predicate, std::chrono::milliseconds timeout = 2000ms) {
        auto start = std::chrono::steady_clock::now();
        while (!predicate()) {
            if (std::chrono::steady_clock::now() - start > timeout) {
                FAIL() << "Timeout waiting for condition";
            }
            std::this_thread::sleep_for(10ms);
        }
    }
};

// 1. Test: Basic Task Execution
TEST_F(TaskQueueTest, ExecutesSingleTask) {
    TaskQueue queue(1);
    bool executed = false;
    std::mutex mtx;

    queue.enqueue([&]() {
        std::lock_guard<std::mutex> lock(mtx);
        executed = true;
    });

    waitForCondition([&]() {
        std::lock_guard<std::mutex> lock(mtx);
        return executed;
    });

    EXPECT_TRUE(executed);
}

// 2. Test: Multiple Sequential Tasks
TEST_F(TaskQueueTest, ExecutesMultipleSequentialTasks) {
    TaskQueue queue(1);  // Use 1 thread to ensure strict sequentiality if needed
    std::atomic<int> counter{0};
    const int num_tasks = 100;

    for (int i = 0; i < num_tasks; ++i) {
        queue.enqueue([&counter]() {
            counter++;
        });
    }

    waitForCondition([&]() {
        return counter == num_tasks;
    });
    EXPECT_EQ(counter.load(), num_tasks);
}

// 3. Test: Concurrency (Default Hardware Concurrency)
TEST_F(TaskQueueTest, HandlesConcurrentTasksWithDefaultThreads) {
    TaskQueue queue(0);  // Triggers getDefaultThreadCount() logic
    std::atomic<int> active_threads{0};
    std::atomic<int> max_observed_concurrency{0};
    const int num_tasks = 50;

    for (int i = 0; i < num_tasks; ++i) {
        queue.enqueue([&]() {
            int current = ++active_threads;
            // Update max observed concurrency
            int observed = max_observed_concurrency.load();
            while (current > observed && !max_observed_concurrency.compare_exchange_weak(observed, current));

            std::this_thread::sleep_for(20ms);  // Hold thread to increase overlap chance
            --active_threads;
        });
    }

    // Wait until all tasks are done (active_threads reaches 0)
    waitForCondition([&]() {
        return active_threads == 0;
    });

    // On multi-core systems, we expect more than 1 thread to be working at once
    EXPECT_GT(max_observed_concurrency.load(), 1);
}

// 4. Test: Stress Test (Heavy Load)
TEST_F(TaskQueueTest, StressTest) {
    TaskQueue queue(8);  // Use a fixed number of threads
    std::atomic<long long> sum{0};
    const int iterations = 1000;

    for (int i = 0; i < iterations; ++i) {
        queue.enqueue([&sum, i]() {
            sum += i;
        });
    }

    // Sum of 0 to 999 is (n*(n-1))/2 => (1000 * 999) / 2 = 499500
    waitForCondition([&]() {
        return sum == 499500;
    });
    EXPECT_EQ(sum.load(), 499500);
}

// 5. Test: Destructor Behavior (Ensures threads join)
TEST_F(TaskQueueTest, ShutsDownCleanly) {
    std::atomic<int> completed_tasks{0};
    {
        TaskQueue queue(4);
        for (int i = 0; i < 10; ++i) {
            queue.enqueue([&]() {
                std::this_thread::sleep_for(10ms);
                completed_tasks++;
            });
        }
    }  // Destructor called here: must wait for all tasks to complete or exit cleanly

    // Depending on implementation, destructor waits for threads to finish current task.
    // Since we used a scope, the queue is destroyed once it's empty and 'stop' is true.
    EXPECT_EQ(completed_tasks.load(), 10);
}

// 6. Test: Manual Thread Count Constraint
TEST_F(TaskQueueTest, RespectsSingleThreadLimit) {
    TaskQueue queue(1);  // Strictly 1 thread
    std::atomic<int> active_threads{0};
    std::atomic<bool> concurrency_violation{false};

    for (int i = 0; i < 20; ++i) {
        queue.enqueue([&]() {
            active_threads++;
            if (active_threads > 1) {
                concurrency_violation = true;
            }
            std::this_thread::sleep_for(15ms);
            active_threads--;
        });
    }

    waitForCondition([&]() {
        return active_threads == 0;
    });
    EXPECT_FALSE(concurrency_violation.load());
}

// 7. Test: Thread Count Fallback (Zero logic)
TEST_F(TaskQueueTest, HandlesZeroThreadRequestWithFallback) {
    // If hardware_concurrency returns 0, it should use 4 as fallback
    // This test checks that the queue still functions with a '0' input
    TaskQueue queue(0);

    std::atomic<bool> task_done{false};
    queue.enqueue([&]() {
        task_done = true;
    });

    waitForCondition([&]() {
        return task_done.load();
    });
    EXPECT_TRUE(task_done.load());
}