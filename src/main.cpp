#include "Executor.hpp"
#include "LinuxNamespaceIsolator.hpp"
#include "Parser.hpp"
#include "PerLanguageRootfsProvider.hpp"
#include "CgroupV2Limiter.hpp"
#include <iostream>
#include <memory>
#include <signal.h>
#include <string>
#include <termios.h>
#include <unistd.h>

int main() {
    auto rootfs_provider = std::make_unique<PerLanguageRootfsProvider>();
    auto linux_isolator = std::make_unique<LinuxNamespaceIsolator>(std::move(rootfs_provider));

    ResourceLimiterFactory limiter_factory = []() {
        // 50% CPU, 50MB RAM, 20 PIDs
        // 50% CPU = 50000us per 100000us period
        return std::make_unique<CgroupV2Limiter>(50000, 100000, 50 * 1024 * 1024, 20);
    };

    Executor executor(std::move(linux_isolator), std::move(limiter_factory));

    struct termios orig_termios;
    tcgetattr(STDIN_FILENO, &orig_termios);
    pid_t isolyx_pgid = getpgrp();

    std::string line;
    while (true) {
        std::cout << "isolyx> ";
        if (!std::getline(std::cin, line))
            break;

        Command cmd = Parser::parseLine(line);
        if (!cmd.isEmpty()) {
            executor.execute(cmd);

            signal(SIGTTOU, SIG_IGN);
            tcsetpgrp(STDIN_FILENO, isolyx_pgid);
            tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
            signal(SIGTTOU, SIG_DFL);
        }
    }
    return 0;
}