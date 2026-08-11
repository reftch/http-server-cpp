#ifndef HTTP_TASK_H
#define HTTP_TASK_H

#include <thread>

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
        void enqueue(F&& fn) {
            // If there's an active thread, signal it to stop and wait for it to die
            if (thread_.joinable()) {
                thread_.request_stop();
                thread_.join();
            }

            // Launch the new task in a fresh thread
            // We use std::forward so that if fn is an rvalue, we move its contents into the lambda
            thread_ = std::jthread([fn = std::forward<F>(fn)](std::stop_token stop) mutable {
                fn(stop);
            });
        }

       private:
        // jthread handles joining on destruction of the Worker object itself
        std::jthread thread_;
    };

}  // namespace http

#endif
