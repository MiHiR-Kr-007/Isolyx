#pragma once
#include "IWatchdog.hpp"
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>

class TimeoutWatchdog : public IWatchdog {
public:
    explicit TimeoutWatchdog(std::chrono::milliseconds timeout);
    ~TimeoutWatchdog() override;

    void start(pid_t pid) override;
    void stop() override;

private:
    std::chrono::milliseconds timeout_;
    pid_t target_pid_ = -1;
    
    std::mutex mtx_;
    std::condition_variable cv_;
    bool running_ = false;
    std::thread watcher_thread_;
};
