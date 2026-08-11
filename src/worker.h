#ifndef HTTP_TASK_H
#define HTTP_TASK_H

// #include <functional>
#include <mutex>
// #include <queue>
#include <thread>
#include <unordered_map>

namespace http {

    class Worker {
       public:
        // Constructor is trivial because no thread starts yet
        Worker() = default;

        /**
         * Swaps the current running task with a new one.
         * This will block the caller until the previous task is finished.
         */
        template <typename F>
            requires std::invocable<F, std::stop_token>
        void enqueue(int id, F&& fn) {
            // Lock the mutex for the entire duration of this operation
            std::lock_guard<std::mutex> lock(threads_mutex_);

            auto it = threads_.find(id);
            if (it != threads_.end()) {
                // Access the jthread via iterator -> second
                // We call request_stop() and join() to ensure the old task is finished
                it->second.request_stop();
                if (it->second.joinable()) {
                    it->second.join();
                }
            }

            // Launch the new task in a fresh thread
            // We use std::forward so that if fn is an rvalue, we move its contents into the lambda
            // auto thread = std::jthread([fn = std::forward<F>(fn)](std::stop_token stop) mutable {
            // fn(stop);
            // });

            // 2. Create the new thread
            // We wrap 'fn' in a lambda that accepts the stop_token
            std::jthread new_thread([fn = std::forward<F>(fn)](std::stop_token stop) mutable {
                fn(stop);
            });

            // Insert/Update the map
            // try_emplace is the best way to move a non-copyable object like jthread into a map
            // threads_.try_emplace(id, std::move(new_thread));

            threads_.erase(id);  // Ensure any existing entry is gone
            threads_.emplace(id, std::move(new_thread));
        }

       private:
        // jthread handles joining on destruction of the Worker object itself
        // std::jthread thread_;
        std::mutex threads_mutex_;
        std::unordered_map<int, std::jthread> threads_;
    };

}  // namespace http

#endif
