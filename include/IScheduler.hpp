#pragma once
#include "Job.hpp"
#include <deque>

class IScheduler {
public:
    virtual ~IScheduler() = default;

    virtual void addJob(std::deque<Job> &jobs, Job job) = 0;

    virtual Job getNextJob(std::deque<Job> &jobs) = 0;
};
