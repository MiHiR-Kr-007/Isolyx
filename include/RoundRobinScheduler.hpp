#pragma once
#include "IScheduler.hpp"

class RoundRobinScheduler : public IScheduler {
public:
    ~RoundRobinScheduler() override = default;
    void addJob(std::deque<Job>& jobs, Job job) override;
    Job getNextJob(std::deque<Job>& jobs) override;
};
