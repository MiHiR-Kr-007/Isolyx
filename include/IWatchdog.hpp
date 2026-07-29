#pragma once
#include <sys/types.h>

class IWatchdog {
public:
    virtual ~IWatchdog() = default;
    
    virtual void start(pid_t pid) = 0;
    virtual void stop() = 0;
};
