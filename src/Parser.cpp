#include "Parser.hpp"
#include <sstream>

Command Parser::parseLine(const std::string &input) {
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
        } else if (token == ">") {
            if (stream >> token) {
                cmd.redirectOutput = token;
            }
        } else if (token == "<") {
            if (stream >> token) {
                cmd.redirectInput = token;
            }
        } else {
            cmd.arguments.push_back(token);
        }
    }

    return cmd;
}

std::vector<Command> Parser::parsePipeline(const std::string &input) {
    std::vector<Command> pipeline;
    std::stringstream stream(input);
    std::string segment;

    while (std::getline(stream, segment, '|')) {
        pipeline.push_back(parseLine(segment));
    }

    return pipeline;
}