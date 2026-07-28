#include <iostream>
#include <string>
#include "Parser.hpp"
#include "Executor.hpp"

int main() {
    std::cout<<"## Isolyx Engine Started\n";

    std::string inputLine;

    // REPL loop
    while (true) {

        Executor::reapZombies();

        std::cout<<"isolyx> ";
        if (!std::getline(std::cin, inputLine)) {
            std::cout<<"\n";
            break; 
        }

        std::vector<Command> pipeline = Parser::parsePipeline(inputLine);
        bool shouldContinue = Executor::executePipeline(pipeline);

        if (!shouldContinue) {
            break;
        }
    }

    std::cout<<"[Isolyx] Engine shutting down.\n";
    return 0;
}