#pragma once
#include "Command.hpp"
#include "IResourceLimiter.hpp"
#include "IWatchdog.hpp"

class IIsolator {
public:
    virtual ~IIsolator() = default;
    virtual int isolateAndRun(const Command &cmd, IResourceLimiter* limiter = nullptr, IWatchdog* watchdog = nullptr) = 0;
};