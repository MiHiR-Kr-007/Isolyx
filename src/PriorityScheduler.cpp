#include "PriorityScheduler.hpp"
#include <algorithm>

void PriorityScheduler::addJob(std::deque<Job>& jobs, Job job) {
    jobs.push_back(std::move(job));
}

Job PriorityScheduler::getNextJob(std::deque<Job>& jobs) {
    auto it = std::max_element(jobs.begin(), jobs.end(), [](const Job& a, const Job& b) {
        return a.priority < b.priority;
    });

    Job job = std::move(*it);
    jobs.erase(it);
    return job;
}
