#pragma once
#include "IScheduler.hpp"

class PriorityScheduler : public IScheduler {
public:
    ~PriorityScheduler() override = default;
    void addJob(std::deque<Job>& jobs, Job job) override;
    Job getNextJob(std::deque<Job>& jobs) override;
};
