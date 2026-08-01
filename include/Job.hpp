#pragma once
#include "Command.hpp"
#include <cstdint>
#include <future>
#include <memory>
#include <chrono>
#include <sys/types.h>

struct Job {
    uint64_t id;
    Command cmd;
    std::shared_ptr<std::promise<int>> result_promise;

    int priority = 0;
    std::chrono::steady_clock::time_point submit_time;
    int user_id = 0;

    pid_t pid = -1;
    int time_quantum = 0;   // 0 means non-preemptive
    int elapsed_bursts = 0; // for MLFQ demotion
};
