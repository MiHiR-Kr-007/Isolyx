#include "LoggingObserver.hpp"
#include <iostream>

void LoggingObserver::onResult(const JobResult& result) {
    std::cout << "[LoggingObserver] Job " << result.job_id 
              << " (" << result.command_line << ") finished."
              << " Verdict: " << result.verdict 
              << ", Exit Code: " << result.exit_code 
              << ", Signal: " << result.term_signal << "\n";
}
