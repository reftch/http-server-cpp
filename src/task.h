#ifndef HTTP_TASK_H
#define HTTP_TASK_H

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace http {

    class TaskQueue {
       public:
        // Parameterized constructor: allows manual setting of worker count
        explicit TaskQueue(size_t thread_count) : stop(false) {
            if (thread_count == 0) thread_count = getDefaultThreadCount();

            workers.reserve(thread_count);

            for (size_t i = 0; i < thread_count; ++i) {
                workers.emplace_back([this] {
                    while (true) {
                        std::function<void()> task;
                        {
                            std::unique_lock<std::mutex> lock(mutex);
                            condition.wait(lock, [this] {
                                return stop || !tasks.empty();
                            });
                            if (stop && tasks.empty()) return;
                            task = std::move(tasks.front());
                            tasks.pop();
                        }
                        task();
                    }
                });
            }
        }

        ~TaskQueue() {
            {
                std::lock_guard<std::mutex> lock(mutex);
                stop = true;
            }

            condition.notify_all();
            for (auto& worker : workers) {
                if (worker.joinable()) worker.join();
            }
        }

        template <typename F>
        void enqueue(F&& f) {
            {
                std::lock_guard<std::mutex> lock(mutex);
                tasks.emplace(std::forward<F>(f));
            }

            condition.notify_one();
        }

       private:
        std::vector<std::thread> workers;
        std::queue<std::function<void()>> tasks;
        std::mutex mutex;
        std::condition_variable condition;
        bool stop;

        // Helper to determine default thread count logic
        static size_t getDefaultThreadCount() {
            auto threads = std::thread::hardware_concurrency();
            return (threads == 0) ? 4 : threads;
        }
    };
}  // namespace http

#endif