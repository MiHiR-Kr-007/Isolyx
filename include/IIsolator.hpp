#pragma once
#include "Command.hpp"
#include "IResourceLimiter.hpp"

class IIsolator {
public:
    virtual ~IIsolator() = default;
    virtual int isolateAndRun(const Command &cmd, IResourceLimiter* limiter = nullptr) = 0;
};