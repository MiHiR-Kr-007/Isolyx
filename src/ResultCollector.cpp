#include "ResultCollector.hpp"

void ResultCollector::addObserver(std::unique_ptr<IResultObserver> observer) {
    std::lock_guard<std::mutex> lock(mutex_);
    observers_.push_back(std::move(observer));
}

void ResultCollector::submitResult(const JobResult& result) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& observer : observers_) {
        observer->onResult(result);
    }
}
