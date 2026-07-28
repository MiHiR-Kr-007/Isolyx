#pragma once
#include "Command.hpp"
#include <string>

class Parser {
public:
    static Command parseLine(const std::string &input);

    // for pipes
    static std::vector<Command> parsePipeline(const std::string &input);
};