#include "Executor.hpp"
#include <iostream>
#include <unistd.h>     
#include <sys/wait.h>  
#include <vector>

bool Executor::handleBuiltin(const Command& cmd) {
    if (cmd.executable == "exit") {
        return false; 
    }
    
    if (cmd.executable == "cd") {
        if (cmd.arguments.size() < 2) {
            std::cerr << "cd: missing argument\n";
        } else {
            if (chdir(cmd.arguments[1].c_str()) != 0) {
                perror("cd failed");
            }
        }
        return true; 
    }

    return true;
}

bool Executor::execute(const Command& cmd) {
    if (cmd.isEmpty()) return true;

    if (cmd.executable == "exit" || cmd.executable == "cd") {
        return handleBuiltin(cmd);
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("Fork failed");
        return true;
    } 
    
    if (pid == 0) {
        // child process

        // converting vector of strings into a array of char pointers
        std::vector<char*> c_args;
        for (const auto& arg : cmd.arguments) {
            c_args.push_back(const_cast<char*>(arg.c_str()));
        }
        c_args.push_back(nullptr); 

        execvp(c_args[0], c_args.data());
        
        std::cerr << "Command not found: " << cmd.executable << "\n";
        exit(1); 
    } else {
        // parent process

        if (!cmd.isBackground) {
            // pause shell until child finishes
            int status;
            waitpid(pid, &status, 0);
        } else {
            std::cout << "[Background process started: PID " << pid << "]\n";
        }
    }

    return true; 
}