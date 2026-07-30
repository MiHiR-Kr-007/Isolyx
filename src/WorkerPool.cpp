#include "WorkerPool.hpp"

WorkerPool::WorkerPool(size_t num_threads, IJobQueue &job_queue, Executor &executor)
    : num_threads_(num_threads), job_queue_(job_queue), executor_(executor) {}

WorkerPool::~WorkerPool() { stop(); }

void WorkerPool::start() {
    if (running_)
        return;
    running_ = true;
    for (size_t i = 0; i < num_threads_; ++i) {
        workers_.emplace_back(&WorkerPool::workerLoop, this);
    }
}

void WorkerPool::stop() {
    if (!running_)
        return;
    running_ = false;
    job_queue_.stop();
    for (auto &worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void WorkerPool::workerLoop() {
    while (running_) {
        auto job_opt = job_queue_.dequeue();
        if (!job_opt) {
            break;
        }

        bool success = executor_.execute(job_opt->cmd);
        if (job_opt->result_promise) {
            job_opt->result_promise->set_value(success ? 0 : 1);
        }
    }
}
