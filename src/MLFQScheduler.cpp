#include "MLFQScheduler.hpp"
#include <algorithm>

void MLFQScheduler::addJob(std::deque<Job> &jobs, Job job) { jobs.push_back(std::move(job)); }

Job MLFQScheduler::getNextJob(std::deque<Job> &jobs) {
    auto it = std::min_element(jobs.begin(), jobs.end(), [](const Job &a, const Job &b) { 
        return (a.priority + a.elapsed_bursts) < (b.priority + b.elapsed_bursts); 
    });

    Job job = std::move(*it);
    jobs.erase(it);
    return job;
}
