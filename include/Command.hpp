#pragma once
#include <string>
#include <vector>

// represent a single command
struct Command {
    std::string executable;
    std::vector<std::string> arguments;
    bool isBackground = false;
    std::string redirectInput;
    std::string redirectOutput;

    bool isEmpty() const { return executable.empty(); }
};