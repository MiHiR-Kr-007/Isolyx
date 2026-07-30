#pragma once
#include "IScheduler.hpp"

class FCFSScheduler : public IScheduler {
public:
    ~FCFSScheduler() override = default;
    void addJob(std::deque<Job>& jobs, Job job) override;
    Job getNextJob(std::deque<Job>& jobs) override;
};
