/**
 * @file ThreadPool.cpp
 * @brief Implementation of thread pool for parallel physics simulation
 */

#include "ThreadPool.h"

namespace quantumverse {
namespace physics {

ThreadPool::ThreadPool(size_t numThreads) {
    for (size_t i = 0; i < numThreads; ++i) {
        threads_.emplace_back([this] {
            while (running_) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(queueMutex_);
                    condition_.wait(lock, [this] { return !tasks_.empty() || !running_; });
                    if (!running_ && tasks_.empty()) break;
                    task = std::move(tasks_.front());
                    tasks_.pop();
                }
                task();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    running_ = false;
    condition_.notify_all();
    for (auto& thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

void ThreadPool::enqueue(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        if (!running_) {
            return;
        }
        tasks_.emplace(std::move(task));
    }
    condition_.notify_one();
}

ThreadPool& getThreadPool() {
    static ThreadPool instance;
    return instance;
}

} // namespace physics
} // namespace quantumverse