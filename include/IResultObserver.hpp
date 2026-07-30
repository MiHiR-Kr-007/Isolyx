#pragma once
#include "JobResult.hpp"

class IResultObserver {
public:
    virtual ~IResultObserver() = default;
    virtual void onResult(const JobResult& result) = 0;
};
