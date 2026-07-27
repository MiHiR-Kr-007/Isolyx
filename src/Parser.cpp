#include "Parser.hpp"
#include <sstream>

Command Parser::parseLine(const std::string& input) {
    Command cmd;
    if (input.empty()) {
        return cmd;
    }

    std::istringstream stream(input);
    std::string token;
    
    // treats first word as the executable
    if (stream >> token) {
        cmd.executable = token;
        cmd.arguments.push_back(token);
    }

    while (stream >> token) {
        if (token == "&") {
            cmd.isBackground = true;
        } else {
            cmd.arguments.push_back(token);
        }
    }

    return cmd;
}