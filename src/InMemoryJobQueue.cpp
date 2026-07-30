#include "InMemoryJobQueue.hpp"

InMemoryJobQueue::InMemoryJobQueue(std::unique_ptr<IScheduler> scheduler)
    : scheduler_(std::move(scheduler)) {}

void InMemoryJobQueue::enqueue(Job job) {
    std::lock_guard<std::mutex> lock(mutex_);
    scheduler_->addJob(jobs_, std::move(job));
    cv_.notify_one();
}

std::optional<Job> InMemoryJobQueue::dequeue() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this]() { return !jobs_.empty() || stopping_; });
    
    if (stopping_ && jobs_.empty()) {
        return std::nullopt;
    }
    
    return scheduler_->getNextJob(jobs_);
}

void InMemoryJobQueue::stop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = true;
    }
    cv_.notify_all();
}
