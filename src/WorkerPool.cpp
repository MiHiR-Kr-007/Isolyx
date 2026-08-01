#include "WorkerPool.hpp"

WorkerPool::WorkerPool(size_t num_threads, IJobQueue &job_queue, Executor &executor, ResultCollector &result_collector)
    : num_threads_(num_threads), job_queue_(job_queue), executor_(executor), result_collector_(result_collector) {}

WorkerPool::~WorkerPool() { stop(); }

void WorkerPool::start() {
    if (running_)
        return;
    running_ = true;
    for (size_t i = 0; i < num_threads_; ++i) {
        workers_.emplace_back(&WorkerPool::workerLoop, this);
    }
}

void WorkerPool::stop() {
    if (!running_)
        return;
    running_ = false;
    job_queue_.stop();
    for (auto &worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void WorkerPool::workerLoop() {
    while (running_) {
        auto job_opt = job_queue_.dequeue();
        if (!job_opt) {
            break;
        }

        ExecutionResult exec_result = executor_.execute(*job_opt);

        if (exec_result.preempted) {
            job_opt->elapsed_bursts++;
            job_queue_.enqueue(std::move(*job_opt));
            continue; // Do not submit result or fulfill promise yet
        }

        JobResult job_result;
        job_result.job_id = job_opt->id;
        job_result.exit_code = exec_result.exit_code;
        job_result.term_signal = exec_result.term_signal;
        job_result.command_line = job_opt->cmd.executable;

        if (exec_result.success) {
            job_result.verdict = "SUCCESS";
        } else if (exec_result.term_signal == 9) {
            job_result.verdict = "TIMEOUT";
        } else if (exec_result.term_signal == 31) {
            job_result.verdict = "SECCOMP_VIOLATION";
        } else if (exec_result.term_signal != 0) {
            job_result.verdict = "KILLED";
        } else {
            job_result.verdict = "FAILED";
        }

        result_collector_.submitResult(job_result);

        if (job_opt->result_promise) {
            job_opt->result_promise->set_value(exec_result.success ? 0 : 1);
        }
    }
}
