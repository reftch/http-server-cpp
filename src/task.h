#ifndef HTTP_TASK_H
#define HTTP_TASK_H

#include <condition_variable>
#include <functional>
#include <list>
#include <mutex>
#include <queue>
#include <set>
#include <thread>
#include <vector>

namespace http {

    class Worker {
       public:
        template <typename F>
        void start(F&& fn) {
            if (thread_.joinable()) {
                thread_.request_stop();
                thread_.join();
            }

            thread_ = std::jthread([fn = std::forward<F>(fn)](std::stop_token stop) mutable {
                fn(stop);
            });
        }

       private:
        std::jthread thread_;
    };

    class TaskQueue {
        // A wrapper to associate a key with the function for removal logic
        struct TaskItem {
            std::string key;
            std::function<void()> func;
        };

       public:
        explicit TaskQueue(size_t thread_count) : stop(false) {
            if (thread_count == 0) thread_count = getDefaultThreadCount();
            for (size_t i = 0; i < thread_count; ++i) {
                workers.emplace_back([this] {
                    while (true) {
                        TaskItem item;
                        {
                            std::unique_lock<std::mutex> lock(mutex);
                            condition.wait(lock, [this] {
                                return stop || !tasks.empty();
                            });
                            if (stop && tasks.empty()) return;
                            item = std::move(tasks.front());
                            tasks.pop_front();
                        }
                        // Execute the actual function
                        item.func();

                        // Cleanup key after execution
                        std::lock_guard<std::mutex> lock(this->mutex);
                        this->running_keys.erase(item.key);
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
            for (auto& worker : workers)
                if (worker.joinable()) worker.join();
        }

        template <typename F>
        void enqueue(const std::string& key, F&& f) {
            std::lock_guard<std::mutex> lock(mutex);
            if (running_keys.find(key) != running_keys.end()) return;

            running_keys.insert(key);
            // Wrap the function so we know its key later for removal/cleanup
            tasks.push_back({key, std::forward<F>(f)});
            condition.notify_one();
        }

        /**
         * @brief Removes a pending task from the queue.
         * @return true if a task was found and removed; false if it was already running or not found.
         */
        bool remove(const std::string& key) {
            if (running_keys.find(key) != running_keys.end()) {
                return false;  // Skip enqueuing
            }

            std::lock_guard<std::mutex> lock(mutex);

            // 1. Check if the key exists at all
            auto it_set = running_keys.find(key);
            if (it_set == running_keys.end()) return false;

            // 2. Try to find the task in the pending list
            for (auto it = tasks.begin(); it != tasks.end(); ++it) {
                if (it->key == key) {
                    tasks.erase(it);             // Remove from queue
                    running_keys.erase(it_set);  // Remove from tracked keys
                    return true;
                }
            }

            // If we reached here, the task is not in the 'tasks' list,
            // which means it has already been popped and is currently running.
            return false;
        }

       private:
        static size_t getDefaultThreadCount() {
            auto threads = std::thread::hardware_concurrency();
            return (threads == 0) ? 4 : threads;
        }

        std::vector<std::thread> workers;
        std::list<TaskItem> tasks;  // Changed to list for easy removal
        std::set<std::string> running_keys;
        std::mutex mutex;
        std::condition_variable condition;
        bool stop;
    };

    // class TaskQueue {
    //    public:
    //     // Parameterized constructor: allows manual setting of worker count
    //     explicit TaskQueue(size_t thread_count) : stop(false) {
    //         if (thread_count == 0) thread_count = getDefaultThreadCount();

    //         workers.reserve(thread_count);

    //         for (size_t i = 0; i < thread_count; ++i) {
    //             workers.emplace_back([this] {
    //                 while (true) {
    //                     std::function<void()> task;
    //                     {
    //                         std::unique_lock<std::mutex> lock(mutex);
    //                         condition.wait(lock, [this] {
    //                             return stop || !tasks.empty();
    //                         });
    //                         if (stop && tasks.empty()) return;
    //                         task = std::move(tasks.front());
    //                         tasks.pop();
    //                     }
    //                     task();
    //                 }
    //             });
    //         }
    //     }

    //     ~TaskQueue() {
    //         {
    //             std::lock_guard<std::mutex> lock(mutex);
    //             stop = true;
    //         }

    //         condition.notify_all();
    //         for (auto& worker : workers) {
    //             if (worker.joinable()) worker.join();
    //         }
    //     }

    //     /**
    //      * @brief Enqueues a task with a unique key.
    //      * If the key is already in the queue, the task is ignored.
    //      */
    //     template <typename F>
    //     void enqueue(const std::string& key, F&& f) {
    //         std::lock_guard<std::mutex> lock(mutex);

    //         // Check if this specific "key" (e.g., "/wstime") is already running/queued
    //         if (running_keys.find(key) != running_keys.end()) {
    //             // return;  // Skip enqueuing
    //         }

    //         running_keys.insert(key);
    //         tasks.emplace([this, key, task_func = std::forward<F>(f)]() mutable {
    //             task_func();
    //             // After the task finishes, remove its key so it can be run again later
    //             std::lock_guard<std::mutex> lock(this->mutex);
    //             this->running_keys.erase(key);
    //         });

    //         condition.notify_one();
    //     }

    //    private:
    //     std::vector<std::thread> workers;
    //     std::queue<std::function<void()>> tasks;
    //     std::set<std::string> running_keys;
    //     std::mutex mutex;
    //     std::condition_variable condition;
    //     bool stop;

    //     // Helper to determine default thread count logic
    //     static size_t getDefaultThreadCount() {
    //         auto threads = std::thread::hardware_concurrency();
    //         return (threads == 0) ? 4 : threads;
    //     }
    // };
}  // namespace http

#endif
