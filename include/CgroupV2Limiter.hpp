#pragma once

#include "IResourceLimiter.hpp"
#include <string>

class CgroupV2Limiter : public IResourceLimiter {
public:
    CgroupV2Limiter(long long max_cpu_us, long long cpu_period_us, long long max_memory_bytes, long long max_pids);
    ~CgroupV2Limiter() override;
    void applyToPid(pid_t pid) override;

private:
    std::string cgroup_path_;
    std::string getBaseCgroupPath();
};
