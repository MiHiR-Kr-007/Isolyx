#pragma once
#include "Command.hpp"

class Executor {
public:
    // executes the parsed command.
    static bool execute(const Command& cmd);

    static bool executePipeline(const std::vector<Command>& pipeline);

    // for cleaning finished background process
    static void reapZombies();

private:
    // for built-in commands like 'cd' or 'exit'
    static bool handleBuiltin(const Command& cmd);
};