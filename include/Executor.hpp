#pragma once
#include "Command.hpp"

class Executor {
public:
    // executes the parsed command.
    static bool execute(const Command& cmd);

private:
    // for built-in commands like 'cd' or 'exit'
    static bool handleBuiltin(const Command& cmd);
};