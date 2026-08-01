#include "DashboardObserver.hpp"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>

DashboardObserver::DashboardObserver(const std::string& pipe_path) : pipe_path_(pipe_path) {
    if (mkfifo(pipe_path_.c_str(), 0666) == -1) {
        if (errno != EEXIST) {
            perror("[DashboardObserver] mkfifo failed");
        }
    }
}

DashboardObserver::~DashboardObserver() {}

void DashboardObserver::writeToPipe(const std::string& data) {
    std::lock_guard<std::mutex> lock(mutex_);
    int fd = open(pipe_path_.c_str(), O_WRONLY | O_NONBLOCK);
    if (fd != -1) {
        write(fd, data.c_str(), data.length());
        write(fd, "\n", 1);
        close(fd);
    }
}

void DashboardObserver::onResult(const JobResult &result) {
    std::string json = "{";
    json += "\"job_id\": " + std::to_string(result.job_id) + ", ";
    json += "\"command_line\": \"" + result.command_line + "\", ";
    json += "\"verdict\": \"" + result.verdict + "\", ";
    json += "\"exit_code\": " + std::to_string(result.exit_code) + ", ";
    json += "\"term_signal\": " + std::to_string(result.term_signal);
    json += "}";
    
    writeToPipe(json);
}
