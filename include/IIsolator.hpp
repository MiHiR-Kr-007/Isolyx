#pragma once
#include "Command.hpp"
#include "IResourceLimiter.hpp"
#include "IWatchdog.hpp"
#include "ISecurityPolicy.hpp"

struct ExecutionResult {
    int exit_code = -1;
    int term_signal = 0;
    bool success = false;
    bool preempted = false;
};

class IIsolator {
public:
    virtual ~IIsolator() = default;
    virtual ExecutionResult isolateAndRun(const Command &cmd, pid_t& pid, int time_quantum, IResourceLimiter* limiter = nullptr, IWatchdog* watchdog = nullptr, ISecurityPolicy* sec_policy = nullptr) = 0;
};