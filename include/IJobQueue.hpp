#pragma once
#include "Job.hpp"
#include <optional>

class IJobQueue {
public:
    virtual ~IJobQueue() = default;
    virtual void enqueue(Job job) = 0;
    virtual std::optional<Job> dequeue() = 0;
    virtual void stop() = 0;
};
