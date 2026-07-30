#pragma once
#include "IJobQueue.hpp"
#include "Executor.hpp"
#include "ResultCollector.hpp"
#include <vector>
#include <thread>
#include <atomic>

class WorkerPool {
public:
    WorkerPool(size_t num_threads, IJobQueue& job_queue, Executor& executor, ResultCollector& result_collector);
    ~WorkerPool();

    void start();
    void stop();

private:
    void workerLoop();

    size_t num_threads_;
    IJobQueue& job_queue_;
    Executor& executor_;
    ResultCollector& result_collector_;
    std::vector<std::thread> workers_;
    std::atomic<bool> running_{false};
};
