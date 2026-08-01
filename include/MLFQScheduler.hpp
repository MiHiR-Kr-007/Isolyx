#pragma once
#include "IScheduler.hpp"

class MLFQScheduler : public IScheduler {
public:
    ~MLFQScheduler() override = default;
    void addJob(std::deque<Job>& jobs, Job job) override;
    Job getNextJob(std::deque<Job>& jobs) override;
};
