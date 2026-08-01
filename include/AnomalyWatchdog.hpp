#pragma once
#include "IWatchdog.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unordered_set>

class AnomalyWatchdog : public IWatchdog {
public:
    AnomalyWatchdog();
    ~AnomalyWatchdog() override;
    
    void start(pid_t pid) override;
    void stop() override;

private:
    pid_t target_pid_ = -1;
    bool running_ = false;
    std::thread watcher_thread_;
    std::mutex mtx_;
    std::condition_variable cv_;
    std::unordered_set<int> suspicious_syscalls_;
};
