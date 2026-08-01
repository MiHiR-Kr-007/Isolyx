#include "Executor.hpp"
#include "LinuxNamespaceIsolator.hpp"
#include "Parser.hpp"
#include "PerLanguageRootfsProvider.hpp"
#include "CgroupV2Limiter.hpp"
#include "TimeoutWatchdog.hpp"
#include "SeccompDenylistPolicy.hpp"
#include "InMemoryJobQueue.hpp"
#include "FCFSScheduler.hpp"
#include "WorkerPool.hpp"
#include "ResultCollector.hpp"
#include "LoggingObserver.hpp"
#include "Job.hpp"
#include "DashboardObserver.hpp"
#include "CompositeWatchdog.hpp"
#include "AnomalyWatchdog.hpp"
#include <iostream>
#include <memory>
#include <signal.h>
#include <string>
#include <termios.h>
#include <unistd.h>
#include <future>

int main() {
    auto rootfs_provider = std::make_unique<PerLanguageRootfsProvider>();
    auto linux_isolator = std::make_unique<LinuxNamespaceIsolator>(std::move(rootfs_provider));

    ResourceLimiterFactory limiter_factory = []() {
        // 50% CPU, 50MB RAM, 20 PIDs
        return std::make_unique<CgroupV2Limiter>(50000, 100000, 50 * 1024 * 1024, 20);
    };

    WatchdogFactory watchdog_factory = []() {
        auto composite = std::make_unique<CompositeWatchdog>();
        // 2 seconds clock timeout
        composite->addWatchdog(std::make_unique<TimeoutWatchdog>(std::chrono::milliseconds(2000)));
        composite->addWatchdog(std::make_unique<AnomalyWatchdog>());
        return composite;
    };

    SecurityPolicyFactory sec_policy_factory = []() {
        return std::make_unique<SeccompDenylistPolicy>();
    };

    Executor executor(std::move(linux_isolator), std::move(limiter_factory), std::move(watchdog_factory), std::move(sec_policy_factory));

    auto scheduler = std::make_unique<FCFSScheduler>();
    InMemoryJobQueue job_queue(std::move(scheduler));

    // pool with 4 worker threads for now...
    ResultCollector result_collector;
    result_collector.addObserver(std::make_unique<LoggingObserver>());
    result_collector.addObserver(std::make_unique<DashboardObserver>("/tmp/isolyx_events"));

    WorkerPool worker_pool(4, job_queue, executor, result_collector);
    worker_pool.start();

    struct termios orig_termios;
    tcgetattr(STDIN_FILENO, &orig_termios);
    pid_t isolyx_pgid = getpgrp();

    uint64_t next_job_id = 1;

    std::string line;
    while (true) {
        std::cout << "isolyx> ";
        if (!std::getline(std::cin, line))
            break;

        Command cmd = Parser::parseLine(line);
        if (!cmd.isEmpty()) {
            if (cmd.executable == "cd" || cmd.executable == "exit") {
                Job j; j.cmd = cmd;
                executor.execute(j);
            } else {
                auto promise = std::make_shared<std::promise<int>>();
                std::future<int> future = promise->get_future();

                Job job{next_job_id++, cmd, promise};
                job_queue.enqueue(std::move(job));

                if (!cmd.isBackground) {
                    future.wait();
                } else {
                    std::cout << "[Background job " << (next_job_id - 1) << " submitted]\n";
                }
            }

            signal(SIGTTOU, SIG_IGN);
            tcsetpgrp(STDIN_FILENO, isolyx_pgid);
            tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
            signal(SIGTTOU, SIG_DFL);
        }
    }

    worker_pool.stop();
    return 0;
}