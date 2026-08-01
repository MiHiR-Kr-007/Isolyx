#pragma once
#include "IResultObserver.hpp"
#include <string>
#include <mutex>

class DashboardObserver : public IResultObserver {
public:
    explicit DashboardObserver(const std::string& pipe_path);
    ~DashboardObserver() override;
    void onResult(const JobResult &result) override;

private:
    std::string pipe_path_;
    std::mutex mutex_;
    void writeToPipe(const std::string& data);
};
