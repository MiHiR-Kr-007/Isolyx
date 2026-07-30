#pragma once
#include "IResultObserver.hpp"
#include "JobResult.hpp"
#include <vector>
#include <memory>
#include <mutex>

class ResultCollector {
public:
    void addObserver(std::unique_ptr<IResultObserver> observer);
    void submitResult(const JobResult& result);

private:
    std::vector<std::unique_ptr<IResultObserver>> observers_;
    std::mutex mutex_;
};
