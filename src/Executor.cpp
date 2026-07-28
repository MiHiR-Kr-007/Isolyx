#include "Executor.hpp"
#include <iostream>
#include <unistd.h>     
#include <sys/wait.h>  
#include <vector>
#include <fcntl.h>

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

        if (!cmd.redirectOutput.empty()) {
            int fdOut = open(cmd.redirectOutput.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fdOut < 0) {
                perror("Failed to open output file");
                exit(1);
            }
            dup2(fdOut, STDOUT_FILENO); 
            close(fdOut);
        }

        if (!cmd.redirectInput.empty()) {
            int fdIn = open(cmd.redirectInput.c_str(), O_RDONLY);
            if (fdIn < 0) {
                perror("Failed to open input file");
                exit(1);
            }
            dup2(fdIn, STDIN_FILENO);
            close(fdIn);
        }

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

void Executor::reapZombies() {
    int status;
    pid_t result;
    
    while ((result = waitpid(-1, &status, WNOHANG)) > 0) {
        std::cout << "[Background process " << result << " finished]\n";
    }
}

bool Executor::executePipeline(const std::vector<Command>& pipeline) {
    if (pipeline.empty()) return true;
    
    if (pipeline.size() == 1) {
        return execute(pipeline[0]);
    }

    int prev_fd = -1; 
    std::vector<pid_t> children;

    for (size_t i = 0; i < pipeline.size(); ++i) {
        const Command& cmd = pipeline[i];
        int fd[2];

        if (i < pipeline.size() - 1) {
            if (pipe(fd) < 0) {
                perror("Pipe failed");
                return true;
            }
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            return true;
        }

        if (pid == 0) {
            // child process

            if (i > 0) {
                dup2(prev_fd, STDIN_FILENO);
                close(prev_fd);
            }

            if (i < pipeline.size() - 1) {
                dup2(fd[1], STDOUT_FILENO);
                close(fd[1]);
                close(fd[0]); 
            }

            if (!cmd.redirectOutput.empty()) {
                int fdOut = open(cmd.redirectOutput.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
                dup2(fdOut, STDOUT_FILENO);
                close(fdOut);
            }
            if (!cmd.redirectInput.empty()) {
                int fdIn = open(cmd.redirectInput.c_str(), O_RDONLY);
                dup2(fdIn, STDIN_FILENO);
                close(fdIn);
            }

            std::vector<char*> c_args;
            for (const auto& arg : cmd.arguments) c_args.push_back(const_cast<char*>(arg.c_str()));
            c_args.push_back(nullptr);
            
            execvp(c_args[0], c_args.data());
            std::cerr << "Command not found: " << cmd.executable << "\n";
            exit(1);
        } else {
            // parent
            children.push_back(pid);

            if (i > 0) {
                close(prev_fd);
            }

            if (i < pipeline.size() - 1) {
                prev_fd = fd[0];
                close(fd[1]); 
            }
        }
    }

    if (!pipeline.back().isBackground) {
        for (pid_t pid : children) {
            int status;
            waitpid(pid, &status, 0);
        }
    }

    return true;
}