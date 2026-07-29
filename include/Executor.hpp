#pragma once
#include "Command.hpp"
#include "IIsolator.hpp"
#include "IResourceLimiter.hpp"
#include <functional>
#include <memory>
#include <vector>

using ResourceLimiterFactory = std::function<std::unique_ptr<IResourceLimiter>()>;

class Executor {
public:
    Executor(std::unique_ptr<IIsolator> isolator, ResourceLimiterFactory limiter_factory = nullptr);

    bool execute(const Command &cmd);
    bool executePipeline(const std::vector<Command> &pipeline);
    void reapZombies();

private:
    std::unique_ptr<IIsolator> isolator_;
    ResourceLimiterFactory limiter_factory_;

    bool handleBuiltin(const Command &cmd);
};