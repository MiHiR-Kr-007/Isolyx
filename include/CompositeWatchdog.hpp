#pragma once
#include "IWatchdog.hpp"
#include <vector>
#include <memory>

class CompositeWatchdog : public IWatchdog {
public:
    CompositeWatchdog();
    ~CompositeWatchdog() override;
    
    void addWatchdog(std::unique_ptr<IWatchdog> watchdog);

    void start(pid_t pid) override;
    void stop() override;

private:
    std::vector<std::unique_ptr<IWatchdog>> watchdogs_;
};
