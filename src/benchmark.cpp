#include <iostream>
#include <vector>
#include <memory>
#include <iomanip>
#include <thread>
#include "Job.hpp"
#include "FCFSScheduler.hpp"
#include "PriorityScheduler.hpp"
#include "RoundRobinScheduler.hpp"
#include "MLFQScheduler.hpp"
#include "InMemoryJobQueue.hpp"
#include "WorkerPool.hpp"
#include "Executor.hpp"
#include "LinuxNamespaceIsolator.hpp"
#include "PerLanguageRootfsProvider.hpp"
#include "LoggingObserver.hpp"

void runRealBenchmark(const std::string& name, std::unique_ptr<IScheduler> scheduler, int quantum) {
    std::cout << "--- Running Real Benchmark for " << name << " ---\n";
    
    auto rootfs = std::make_unique<PerLanguageRootfsProvider>();
    auto isolator = std::make_unique<LinuxNamespaceIsolator>(std::move(rootfs));
    Executor executor(std::move(isolator));
    
    ResultCollector collector;
    collector.addObserver(std::make_unique<LoggingObserver>());
    
    InMemoryJobQueue queue(std::move(scheduler));
    
    WorkerPool pool(1, queue, executor, collector);
    pool.start();
    
    auto start_time = std::chrono::steady_clock::now();
    
    std::vector<std::shared_ptr<std::promise<int>>> promises;
    
    for (int i = 0; i < 3; ++i) {
        Job j;
        j.id = i + 1;
        j.priority = i;
        j.time_quantum = quantum;
        j.cmd.executable = "/cpu_hog"; 
        
        auto prom = std::make_shared<std::promise<int>>();
        j.result_promise = prom;
        promises.push_back(prom);
        
        queue.enqueue(std::move(j));
    }
    
    for (auto& prom : promises) {
        prom->get_future().get(); 
    }
    
    pool.stop();
    auto end_time = std::chrono::steady_clock::now();
    std::cout << "Total execution time: " 
              << std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() 
              << " ms\n\n";
}

int main() {
    std::cout << "--- Isolyx REAL Scheduler Benchmarking ---\n\n";
    
    // FCFS (No Preemption, quantum = 0)
    runRealBenchmark("FCFS", std::make_unique<FCFSScheduler>(), 0);
    
    // RoundRobin (Preemptive, quantum = 2000 ms)
    runRealBenchmark("RoundRobin", std::make_unique<RoundRobinScheduler>(), 2000);
    
    // MLFQ (Preemptive, quantum = 2000 ms)
    runRealBenchmark("MLFQ", std::make_unique<MLFQScheduler>(), 2000);

    return 0;
}
