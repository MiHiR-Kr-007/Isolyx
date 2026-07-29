#include "TimeoutWatchdog.hpp"
#include <csignal>
#include <unistd.h>

TimeoutWatchdog::TimeoutWatchdog(std::chrono::milliseconds timeout) : timeout_(timeout) {}

TimeoutWatchdog::~TimeoutWatchdog() { stop(); }

void TimeoutWatchdog::start(pid_t pid) {
    target_pid_ = pid;
    running_ = true;
    watcher_thread_ = std::thread([this]() {
        std::unique_lock<std::mutex> lock(mtx_);
        if (cv_.wait_for(lock, timeout_, [this] { return !running_; })) {
            return;
        }

        if (target_pid_ > 0) {
            kill(-target_pid_, SIGKILL);
            kill(target_pid_, SIGKILL);
        }
    });
}

void TimeoutWatchdog::stop() {
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (!running_)
            return;
        running_ = false;
    }
    cv_.notify_one();
    if (watcher_thread_.joinable()) {
        watcher_thread_.join();
    }
}
