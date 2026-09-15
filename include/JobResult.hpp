#pragma once
#include <string>
#include <cstdint>

struct JobResult {
    uint64_t job_id;
    int exit_code;
    int term_signal;
    std::string verdict; // "SUCCESS", "TIMEOUT", "SECCOMP_VIOLATION", "KILLED", "ERROR"
    std::string command_line;
    std::string stdout_out;
    std::string stderr_out;
};
