#pragma once
#include "Command.hpp"
#include "IIsolator.hpp"
#include <memory>
#include <vector>

class Executor {
public:
    explicit Executor(std::unique_ptr<IIsolator> isolator);

    bool execute(const Command &cmd);
    bool executePipeline(const std::vector<Command> &pipeline);
    void reapZombies();

private:
    std::unique_ptr<IIsolator> isolator_;

    bool handleBuiltin(const Command &cmd);
};