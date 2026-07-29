#include "Executor.hpp"
#include <fcntl.h>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

Executor::Executor(std::unique_ptr<IIsolator> isolator, ResourceLimiterFactory limiter_factory)
    : isolator_(std::move(isolator)), limiter_factory_(std::move(limiter_factory)) {}

bool Executor::execute(const Command &cmd) {
    if (cmd.isEmpty())
        return true;

    if (cmd.executable == "cd" || cmd.executable == "exit") {
        return handleBuiltin(cmd);
    }
    
    auto limiter = limiter_factory_ ? limiter_factory_() : nullptr;
    int exit_code = isolator_->isolateAndRun(cmd, limiter.get());

    return exit_code == 0;
}

bool Executor::handleBuiltin(const Command &cmd) {
    if (cmd.executable == "exit") {
        exit(0);
    } else if (cmd.executable == "cd") {
        if (cmd.arguments.size() < 2) {
            std::cerr << "cd: missing argument" << std::endl;
        } else {
            if (chdir(cmd.arguments[1].c_str()) != 0) {
                perror("cd failed");
            }
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

bool Executor::executePipeline(const std::vector<Command> &pipeline) {
    if (pipeline.empty())
        return true;

    if (pipeline.size() == 1) {
        return execute(pipeline[0]);
    }

    int prev_fd = -1;
    std::vector<pid_t> children;

    for (size_t i = 0; i < pipeline.size(); ++i) {
        const Command &cmd = pipeline[i];
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

            std::vector<char *> c_args;
            for (const auto &arg : cmd.arguments)
                c_args.push_back(const_cast<char *>(arg.c_str()));
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