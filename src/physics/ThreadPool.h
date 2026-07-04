/**
 * @file ThreadPool.h
 * @brief Thread pool for parallel physics simulation
 *
 * Provides a simple thread pool for distributing physics calculations
 * across multiple CPU cores for improved performance.
 */

#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

namespace quantumverse {
namespace physics {

/**
 * @brief Simple thread pool for parallel task execution
 */
class ThreadPool {
public:
    /**
     * @brief Construct thread pool with specified number of threads
     */
    explicit ThreadPool(size_t numThreads = 4);

    /**
     * @brief Destructor - waits for all tasks to complete
     */
    ~ThreadPool();

    /**
     * @brief Submit a task to the pool
     */
    void enqueue(std::function<void()> task);

    /**
     * @brief Get number of threads
     */
    size_t size() const { return threads_.size(); }

    /**
     * @brief Check if pool is running
     */
    bool isRunning() const { return running_; }

private:
    std::vector<std::thread> threads_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queueMutex_;
    std::condition_variable condition_;
    std::atomic<bool> running_{true};
};

/**
 * @brief Get global thread pool instance
 */
ThreadPool& getThreadPool();

} // namespace physics
} // namespace quantumverse