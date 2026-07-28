#pragma once
#include "Command.hpp"

class IIsolator {
public:
    virtual ~IIsolator() = default;
    virtual int isolateAndRun(const Command &cmd) = 0;
};