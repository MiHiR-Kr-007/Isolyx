#pragma once
#include "IResultObserver.hpp"

class LoggingObserver : public IResultObserver {
public:
    void onResult(const JobResult& result) override;
};
