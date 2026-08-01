#include "CompositeWatchdog.hpp"

CompositeWatchdog::CompositeWatchdog() {}
CompositeWatchdog::~CompositeWatchdog() {}

void CompositeWatchdog::addWatchdog(std::unique_ptr<IWatchdog> watchdog) {
    if (watchdog) {
        watchdogs_.push_back(std::move(watchdog));
    }
}

void CompositeWatchdog::start(pid_t pid) {
    for (auto& w : watchdogs_) {
        w->start(pid);
    }
}

void CompositeWatchdog::stop() {
    for (auto& w : watchdogs_) {
        w->stop();
    }
}
