#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>
#include <stdexcept>

namespace quantumverse {
namespace utils {

class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency())
        : stop_flag_(false) {
        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this] { workerLoop(); });
        }
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            stop_flag_ = true;
        }
        condition_.notify_all();
        for (auto& worker : workers_) {
            if (worker.joinable()) worker.join();
        }
    }

    template<typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::invoke_result_t<F, Args...>> {
        using ReturnType = typename std::invoke_result_t<F, Args...>;

        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<ReturnType> result = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            if (stop_flag_) {
                throw std::runtime_error("ThreadPool is stopped");
            }
            tasks_.emplace([task]() { (*task)(); });
        }
        condition_.notify_one();
        return result;
    }

    void parallel_for(size_t start, size_t end, 
                      const std::function<void(size_t)>& func,
                      size_t num_chunks = 0) {
        if (start >= end) return;
        
        size_t total = end - start;
        size_t chunks = num_chunks > 0 ? num_chunks : workers_.size();
        size_t chunk_size = (total + chunks - 1) / chunks;

        std::vector<std::future<void>> futures;
        for (size_t c = 0; c < chunks; ++c) {
            size_t chunk_start = start + c * chunk_size;
            size_t chunk_end = std::min(chunk_start + chunk_size, end);
            futures.push_back(enqueue([chunk_start, chunk_end, func]() {
                for (size_t i = chunk_start; i < chunk_end; ++i) {
                    func(i);
                }
            }));
        }

        for (auto& fut : futures) {
            fut.wait();
        }
    }

    size_t size() const { return workers_.size(); }

private:
    void workerLoop() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(queue_mutex_);
                condition_.wait(lock, [this] { return stop_flag_ || !tasks_.empty(); });
                
                if (stop_flag_ && tasks_.empty()) return;
                
                task = std::move(tasks_.front());
                tasks_.pop();
            }
            task();
        }
    }

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_flag_;
};

} // namespace utils
} // namespace quantumverse