#ifndef HTTP_POOL_H
#define HTTP_POOL_H

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace http {

    class ThreadPool {
       public:
        explicit ThreadPool() : stop(false) {
            auto threads = std::thread::hardware_concurrency();

            if (threads == 0) threads = 4;
            workers.reserve(threads);

            for (size_t i = 0; i < threads; ++i) {
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

        ~ThreadPool() {
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
    };
}  // namespace http

#endif