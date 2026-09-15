#include "DashboardObserver.hpp"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include "json.hpp"

using json = nlohmann::json;

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
    json j;
    j["job_id"] = result.job_id;
    j["command_line"] = result.command_line;
    j["verdict"] = result.verdict;
    j["exit_code"] = result.exit_code;
    j["term_signal"] = result.term_signal;
    j["stdout_out"] = result.stdout_out;
    j["stderr_out"] = result.stderr_out;
    
    writeToPipe(j.dump());
}
