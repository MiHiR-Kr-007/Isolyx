#pragma once
#include <sys/types.h>

class IResourceLimiter {
public:
    virtual ~IResourceLimiter() = default;

    virtual void applyToPid(pid_t pid) = 0;
};
