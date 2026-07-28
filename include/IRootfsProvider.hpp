#pragma once
#include <string>

class IRootfsProvider {
public:
    virtual ~IRootfsProvider() = default;

    virtual std::string prepareRootfs() = 0;

    virtual void teardownRootfs() = 0;
};