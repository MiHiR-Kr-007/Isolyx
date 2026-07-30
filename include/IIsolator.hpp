#pragma once
#include "Command.hpp"
#include "IResourceLimiter.hpp"
#include "IWatchdog.hpp"
#include "ISecurityPolicy.hpp"

class IIsolator {
public:
    virtual ~IIsolator() = default;
    virtual int isolateAndRun(const Command &cmd, IResourceLimiter* limiter = nullptr, IWatchdog* watchdog = nullptr, ISecurityPolicy* sec_policy = nullptr) = 0;
};