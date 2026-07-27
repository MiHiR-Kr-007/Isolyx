#include <iostream>
#include <string>
#include "Parser.hpp"
#include "Executor.hpp"

int main() {
    std::cout<<"## Isolyx Engine Started\n";

    std::string inputLine;

    // REPL loop
    while (true) {
        std::cout << "isolyx> ";
        if (!std::getline(std::cin, inputLine)) {
            std::cout<<"\n";
            break; 
        }

        Command cmd = Parser::parseLine(inputLine);

        bool shouldContinue = Executor::execute(cmd);

        if (!shouldContinue) {
            break;
        }
    }

    std::cout<<"[Isolyx] Engine shutting down.\n";
    return 0;
}