#include "FCFSScheduler.hpp"

void FCFSScheduler::addJob(std::deque<Job>& jobs, Job job) {
    jobs.push_back(std::move(job));
}

Job FCFSScheduler::getNextJob(std::deque<Job>& jobs) {
    Job job = std::move(jobs.front());
    jobs.pop_front();
    return job;
}
