#pragma once
#include "Command.hpp"
#include <string>

class Parser {
public:
    // takes raw line typed from user and parses it into a Command struct.
    static Command parseLine(const std::string& input);
};