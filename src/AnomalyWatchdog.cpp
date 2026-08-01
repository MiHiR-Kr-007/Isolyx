#include "AnomalyWatchdog.hpp"
#include <fstream>
#include <csignal>
#include <iostream>

AnomalyWatchdog::AnomalyWatchdog() {
    // Syscalls like 41=socket, 42=connect, 43=accept, 44=sendto, 45=recvfrom
    suspicious_syscalls_ = {41, 42, 43, 44, 45, 46, 47, 49, 50};
}

AnomalyWatchdog::~AnomalyWatchdog() { stop(); }

void AnomalyWatchdog::start(pid_t pid) {
    target_pid_ = pid;
    running_ = true;
    watcher_thread_ = std::thread([this]() {
        while (true) {
            {
                std::unique_lock<std::mutex> lock(mtx_);
                if (cv_.wait_for(lock, std::chrono::milliseconds(20), [this] { return !running_; })) {
                    return; // Stopped
                }
            }

            if (target_pid_ <= 0) break;

            std::string path = "/proc/" + std::to_string(target_pid_) + "/syscall";
            std::ifstream file(path);
            if (file.is_open()) {
                long syscall_num = -1;
                if (file >> syscall_num) {
                    if (suspicious_syscalls_.count(syscall_num)) {
                        std::cerr << "\n[AnomalyWatchdog] Profiler detected process blocked on suspicious syscall " 
                                  << syscall_num << "! Pattern anomaly. Terminating...\n";
                        kill(-target_pid_, SIGKILL);
                        kill(target_pid_, SIGKILL);
                        return; 
                    }
                }
            }
        }
    });
}

void AnomalyWatchdog::stop() {
    {
        std::lock_guard<std::mutex> lock(mtx_);
        if (!running_) return;
        running_ = false;
    }
    cv_.notify_one();
    if (watcher_thread_.joinable()) {
        watcher_thread_.join();
    }
}
