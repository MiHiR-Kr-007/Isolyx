#include "RoundRobinScheduler.hpp"

void RoundRobinScheduler::addJob(std::deque<Job>& jobs, Job job) {
    jobs.push_back(std::move(job));
}

Job RoundRobinScheduler::getNextJob(std::deque<Job>& jobs) {
    Job job = std::move(jobs.front());
    jobs.pop_front();
    return job;
}
