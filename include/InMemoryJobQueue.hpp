#pragma once
#include "IJobQueue.hpp"
#include "IScheduler.hpp"
#include <mutex>
#include <condition_variable>
#include <memory>
#include <deque>

class InMemoryJobQueue : public IJobQueue {
public:
    explicit InMemoryJobQueue(std::unique_ptr<IScheduler> scheduler);
    ~InMemoryJobQueue() override = default;
    
    void enqueue(Job job) override;
    std::optional<Job> dequeue() override;
    void stop() override;

private:
    std::unique_ptr<IScheduler> scheduler_;
    std::deque<Job> jobs_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool stopping_{false};
};
